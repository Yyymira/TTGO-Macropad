"""Servicio de fondo para Linux: maneja la placa (serial, capas, inyección de
teclas, stats) sin ventana y expone una API HTTP local para el widget de
Plasma (`plasmoid/`).

La API escucha solo en 127.0.0.1 y exige un token aleatorio que se regenera
en cada arranque y se guarda en $XDG_RUNTIME_DIR/ttgo-macropad/token (carpeta
0700): solo este usuario puede leerlo, así que otros usuarios del equipo no
pueden reconfigurar las teclas.

    GET  /status  -> conexión, modo, capa activa, nombres de capas, stats
    GET  /config  -> configuración completa
    POST /config  -> reemplaza y guarda la configuración
    POST /layer   -> {"index": n}: activa la capa n en la placa
"""

import json
import os
import secrets
import signal
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from PySide6.QtCore import QCoreApplication, QObject, QTimer, Signal

sys.path.insert(0, str(Path(__file__).resolve().parent))

from macropad.active_window import start_active_window_watcher  # noqa: E402
from macropad.browser import BrowserLink  # noqa: E402
from macropad.config import DEFAULT_DEVICE  # noqa: E402
from macropad.controller import Controller  # noqa: E402
from macropad.geocode import CityNotFound, geocode  # noqa: E402
from macropad.i18n import tr  # noqa: E402
from macropad.serial_link import SerialLink  # noqa: E402

PORT = 47823
MAX_BODY = 256 * 1024
RUNTIME_DIR = Path(os.environ.get("XDG_RUNTIME_DIR", f"/run/user/{os.getuid()}")) / "ttgo-macropad"


class Bridge(QObject):
    """Pasa pedidos del hilo HTTP al hilo principal de Qt (conexión encolada)."""

    config_requested = Signal(dict)
    layer_requested = Signal(int)


def write_token() -> str:
    token = secrets.token_urlsafe(32)
    RUNTIME_DIR.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(RUNTIME_DIR, 0o700)
    path = RUNTIME_DIR / "token"
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w") as f:
        f.write(token)
    return token


def valid_config(cfg) -> bool:
    if not isinstance(cfg, dict):
        return False
    layers = cfg.get("layers")
    if not isinstance(layers, list) or not layers:
        return False
    if not isinstance(cfg.get("device", {}), dict):
        return False
    return all(
        isinstance(l, dict) and isinstance(l.get("buttons"), list) and isinstance(l.get("encoders"), list)
        for l in layers
    )


def resolve_city(cfg: dict, current: dict) -> str | None:
    """Completa lat/lon/place si cambió la ciudad. Devuelve un error legible o None.
    Se hace acá (hilo HTTP) y no en el hilo de Qt porque es una consulta de red."""
    device = {**DEFAULT_DEVICE, **current, **cfg.get("device", {})}
    if not device.get("ota_pass"):
        device["ota_pass"] = current.get("ota_pass", "")
    city = (device.get("city") or "").strip()
    device["city"] = city
    if not city:
        device.update(lat=None, lon=None, place="")
    elif city != current.get("city") or device.get("lat") is None:
        try:
            device["lat"], device["lon"], device["place"] = geocode(city)
        except CityNotFound:
            return tr("Couldn't find the city “{city}”", city=city)
        except OSError:
            return tr("Couldn't look up the city (no internet?)")
    cfg["device"] = device
    return None


def make_handler(token: str, link: SerialLink, controller: Controller, bridge: Bridge):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):  # sin spam en el journal
            pass

        def _send(self, code: int, obj=None) -> None:
            body = b"" if obj is None else json.dumps(obj, ensure_ascii=False).encode()
            self.send_response(code)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def _authorized(self) -> bool:
            sent = self.headers.get("Authorization", "")
            if secrets.compare_digest(sent, f"Bearer {token}"):
                return True
            self._send(401, {"error": tr("invalid token")})
            return False

        def _read_json(self):
            length = int(self.headers.get("Content-Length", 0))
            if not 0 < length <= MAX_BODY:
                return None
            try:
                return json.loads(self.rfile.read(length))
            except (json.JSONDecodeError, UnicodeDecodeError):
                return None

        # --- extensión del navegador: autenticada con su propia clave ---
        def _browser_authorized(self) -> bool:
            if controller.browser.check_key(self.headers.get("X-Macropad-Key", "")):
                return True
            self._send(401, {"error": tr("invalid token")})
            return False

        def _browser_request(self, method: str) -> bool:
            """Atiende /browser/*; devuelve False si la ruta no es de la extensión."""
            browser = controller.browser
            if method == "POST" and self.path == "/browser/pair":
                key = browser.pair()
                self._send(200 if key else 403, {"key": key} if key else {"error": "pairing closed"})
            elif method == "POST" and self.path == "/browser/state":
                if self._browser_authorized():
                    data = self._read_json()
                    if isinstance(data, dict):
                        browser.state_received.emit(data)
                    self._send(200, {"ok": True})
            elif method == "GET" and self.path == "/browser/poll":
                if self._browser_authorized():
                    self._send(200, {"commands": browser.poll()})
            else:
                return False
            return True

        def do_GET(self):
            if self.path.startswith("/browser/"):
                if not self._browser_request("GET"):
                    self._send(404, {"error": tr("not found")})
                return
            if not self._authorized():
                return
            if self.path == "/status":
                layer = controller.current_layer
                self._send(200, {
                    "connected": link.connected,
                    "mode": controller.mode,
                    "layer_index": controller.layer_index,
                    "layer_name": layer.get("name", ""),
                    "layers": [l.get("name", "") for l in controller.config["layers"]],
                    "stats": controller.last_stats,
                    "device_net": controller.device_net if link.connected else {},
                    "active_app": controller.active_app,
                    "browser": {"connected": controller.browser.connected(),
                                "pairing": controller.browser.pairing_open(),
                                "site": controller.browser_site,
                                "targets": controller.browser_targets,
                                "media": controller.browser_media_summary()},
                })
            elif self.path == "/config":
                self._send(200, controller.config)
            else:
                self._send(404, {"error": tr("not found")})

        def do_POST(self):
            if self.path.startswith("/browser/") and self.path != "/browser/pairing":
                if not self._browser_request("POST"):
                    self._send(404, {"error": tr("not found")})
                return
            if not self._authorized():
                return
            if self.path == "/browser/pairing":  # el widget abre la ventana de vinculación
                controller.browser.open_pairing()
                self._send(200, {"ok": True})
                return
            data = self._read_json()
            if self.path == "/config":
                if not valid_config(data):
                    self._send(400, {"error": tr("invalid configuration")})
                    return
                error = resolve_city(data, controller.config.get("device", {}))
                if error:
                    self._send(400, {"error": error})
                    return
                bridge.config_requested.emit(data)
                self._send(200, {"ok": True, "device": data["device"]})
            elif self.path == "/layer":
                if not isinstance(data, dict) or not isinstance(data.get("index"), int):
                    self._send(400, {"error": tr("missing index")})
                    return
                bridge.layer_requested.emit(data["index"])
                self._send(200, {"ok": True})
            else:
                self._send(404, {"error": tr("not found")})

    return Handler


def main() -> None:
    app = QCoreApplication(sys.argv)

    link = SerialLink()
    controller = Controller(link)
    bridge = Bridge()
    bridge.config_requested.connect(controller.reload_config)
    bridge.layer_requested.connect(controller.select_layer)
    window_watcher = start_active_window_watcher(controller)  # noqa: F841 (hay que mantenerlo vivo)

    token = write_token()
    server = ThreadingHTTPServer(("127.0.0.1", PORT), make_handler(token, link, controller, bridge))
    threading.Thread(target=server.serve_forever, daemon=True).start()

    # Qt no le devuelve el control a Python mientras espera eventos; este
    # timer vacío permite que SIGTERM (systemctl stop) se procese a tiempo.
    signal.signal(signal.SIGTERM, lambda *_: app.quit())
    signal.signal(signal.SIGINT, lambda *_: app.quit())
    keepalive = QTimer()
    keepalive.timeout.connect(lambda: None)
    keepalive.start(500)

    link.start()
    controller.start()
    exit_code = app.exec()

    # Si cerrar el puerto serie se cuelga (lectura bloqueada en otro hilo),
    # no dejamos el proceso zombi: a los 3 s se sale igual.
    failsafe = threading.Timer(3, lambda: os._exit(exit_code))
    failsafe.daemon = True
    failsafe.start()
    server.shutdown()
    controller.shutdown()
    link.stop()
    (RUNTIME_DIR / "token").unlink(missing_ok=True)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
