"""Cerebro de la app: máquina de estados de capas, timer de inactividad,
despacho de eventos de botones/encoders a acciones, y empuje continuo de
stats del sistema a la pantalla idle de la placa."""

import re
import time

from PySide6.QtCore import QObject, Signal, QTimer

from macropad.browser import BrowserLink
from macropad.config import load_config, save_config
from macropad.i18n import LANG, tr
from macropad.injector import KeyInjector
from macropad.media import ArtLoader, MediaReader, art_chunks, ascii_text, media_title
from macropad.runner import run_command
from macropad.serial_link import SerialLink
from macropad.stats import StatsReader

STATS_PUSH_INTERVAL_MS = 1000
# Tras "cambiar pantalla de reposo" (suele estar en el pulsador de un encoder,
# y al apretar la perilla se gira un poco), los giros no muestran la capa.
VIEW_PRESS_QUIET_S = 0.5


# Acción "idle_view": cada pulsación fija la pantalla de reposo en la siguiente
# (None = la configurada). Es momentáneo: no se guarda en la config.
VIEW_CYCLE = [None, "stats", "weather", "media"]
VIEW_NAMES = {None: "Auto", "stats": "Stats", "weather": "Weather", "media": "Media"}
# La placa confirma cada pedazo de la tapa ("art_ack") y recién ahí va el
# siguiente; si no confirma a tiempo (se perdió o llegó dañado), se reenvía.
ART_RETRY_MS = 400
ART_MAX_TRIES = 8


def action_label(action: dict, view_override: str | None = None) -> str:
    kind = action.get("action", "none")
    if kind == "idle_view":
        return tr("View: {v}", v=tr(VIEW_NAMES[view_override]))
    if kind == "key":
        return "+".join(k.upper() for k in action.get("keys", []))[:14]
    if kind == "layer_next":
        return tr("Layer >>")
    if kind == "layer_prev":
        return tr("<< Layer")
    if kind == "layer_set":
        return tr("Layer {n}", n=action.get("index", 0))
    if kind == "command":
        return action.get("cmd", "").strip()[:14]
    if kind == "browser":
        return tr(BROWSER_LABELS.get(action.get("cmd", ""), ""))
    return ""


def encoder_is_command(mapping: dict) -> bool:
    return mapping.get("type") == "command"


def encoder_sends_keys(mapping: dict) -> bool:
    return mapping.get("type", "keys") == "keys"


# Acciones "Navegador" (las ejecuta la extensión): etiqueta corta para la pantalla.
BROWSER_LABELS = {
    "speed_up": "Speed +", "speed_down": "Speed -", "speed_reset": "Speed 1x",
    "seek_forward": ">> 10s", "seek_back": "<< 10s", "skip_ad": "Skip ad",
    "tab_volume_up": "Tab vol +", "tab_volume_down": "Tab vol -", "focus_media_tab": "Go to tab",
    "toggle_mic": "Mic", "toggle_cam": "Camera",
}
BROWSER_PLAYERS = ("brave", "chrom", "firefox", "vivaldi", "opera", "edge", "plasma-browser-integration")
BROWSER_MEDIA_MAX_AGE_S = 5  # datos de la extensión más viejos que esto se ignoran


def _same_title(a: str, b: str) -> bool:
    """¿El título del sistema (MPRIS) y el de la extensión son del mismo video?
    El del sistema suele ser el de la pestaña: "(38) Tema - YouTube"."""
    def norm(t: str) -> str:
        t = re.sub(r"^\(\d+\)\s*|\s+-\s+youtube( music)?$", "", ascii_text(t).lower())
        return re.sub(r"[^a-z0-9]+", " ", t).strip()
    a, b = norm(a), norm(b)
    return bool(a and b) and (a in b or b in a)


def layer_to_phone(layer: dict) -> bool:
    """Capa con destino "celular": sus teclas salen por el Bluetooth de la placa."""
    return layer.get("target") == "phone"


def _compact_action(action: dict) -> dict:
    kind = action.get("action", "none")
    out = {"a": kind}
    if kind == "key":
        out["k"] = action.get("keys", [])
    elif kind == "layer_set":
        out["i"] = action.get("index", 0)
    return out


def keymap_message(config: dict) -> dict:
    """Copia compacta de las capas para el modo Bluetooth de la placa."""
    layers = [
        {
            "n": layer.get("name", ""),
            "b": [_compact_action(a) for a in layer.get("buttons", [])],
            # los comandos solo existen con la PC: en Bluetooth ese encoder no manda nada
            "e": [
                {"cw": e.get("cw", []), "ccw": e.get("ccw", [])} if encoder_sends_keys(e) else {"cw": [], "ccw": []}
                for e in layer.get("encoders", [])
            ],
        }
        for layer in config["layers"]
    ]
    return {"type": "keymap", "layers": layers}


def settings_message(config: dict) -> dict:
    device = config.get("device", {})
    msg = {
        "type": "settings",
        "wifi_ssid": device.get("wifi_ssid", ""),
        "wifi_pass": device.get("wifi_pass", ""),
        "ota_pass": device.get("ota_pass", ""),
        "idle_view": device.get("idle_view", "both"),
        "idle_timeout": config.get("idle_timeout_sec", 15),
        "utc_offset": time.localtime().tm_gmtoff,  # hora local mientras el clima no la confirma
        "lang": LANG,  # idioma de la pantalla de la placa
        "stats_secs": device.get("stats_secs", 10),
        "weather_secs": device.get("weather_secs", 10),
        "media_secs": device.get("media_secs", 10),
    }
    if device.get("lat") is not None and device.get("lon") is not None:
        msg["lat"] = device["lat"]
        msg["lon"] = device["lon"]
    return msg


class Controller(QObject):
    layer_changed = Signal(int, str)  # index, name — para refrescar la UI
    mode_changed = Signal(str)  # "idle" | "layer"

    def __init__(self, link: SerialLink):
        super().__init__()
        self.link = link
        self.config = load_config()
        self.injector = KeyInjector()
        self.stats = StatsReader()

        self.layer_index = 0
        self.mode = "idle"
        self.last_stats: dict = {}
        self.device_net: dict = {}  # WiFi/IP/Bluetooth/clima, reportado por la placa
        self._activity_ticks_left = 0  # en unidades de STATS_PUSH_INTERVAL_MS

        # Capas por programa: la ventana activa puede elegir la capa sola.
        self.active_app = ""  # programa en primer plano (lo muestra el widget)
        self._active_ids: list[str] = []
        self._app_match = -1  # capa que corresponde a la ventana actual (-1 = ninguna)
        self._auto_layer = False  # la capa activa la eligió la ventana, no el usuario
        self._manual_layer = 0  # a la que se vuelve al salir del programa
        self.view_override: str | None = None  # pantalla de reposo fijada con un botón
        self._quiet_until = 0.0  # hasta cuándo los giros no cuentan como actividad

        # Pantalla de multimedia: qué suena y la tapa
        self.media = MediaReader()
        self.art_loader = ArtLoader()
        self.art_loader.loaded.connect(self._on_art_loaded)
        self._last_media_msg: dict | None = None
        self._art_url = ""
        self._art_id = 0  # id de la tapa actual en la placa (0 = sin tapa)
        self._art_counter = 0
        self._art_data: bytes | None = None
        self._art_queue: list[dict] = []
        self._art_timer = QTimer(self)
        self._art_timer.setInterval(ART_RETRY_MS)
        self._art_timer.timeout.connect(self._send_art_chunk)
        self._art_tries = 0

        # Extensión del navegador: sitio al frente, multimedia y reunión
        self.browser = BrowserLink()
        self.browser.state_received.connect(self._on_browser_state)
        self._browser_site = ""
        self._browser_media: dict | None = None
        self._browser_media_at = 0.0
        self._meeting: dict = {}

        self.timer = QTimer(self)
        self.timer.setInterval(STATS_PUSH_INTERVAL_MS)
        self.timer.timeout.connect(self._tick)

        self.link.event_received.connect(self._on_device_event)

    def start(self) -> None:
        self.timer.start()

    @property
    def current_layer(self) -> dict:
        layers = self.config["layers"]
        self.layer_index = max(0, min(self.layer_index, len(layers) - 1))
        return layers[self.layer_index]

    def reload_config(self, new_config: dict) -> None:
        self.config = new_config
        save_config(self.config)
        self.layer_index = min(self.layer_index, len(self.config["layers"]) - 1)
        self._manual_layer = min(self._manual_layer, len(self.config["layers"]) - 1)
        self._sync_device()
        if self.mode == "layer":
            self._send_layer_snapshot()
        self._app_match = -1  # pudieron cambiar los programas de cada capa
        self._apply_app_layer()

    # ---------------- capas por programa ----------------

    def set_active_window(self, ids: list[str]) -> None:
        """La ventana activa cambió. `ids` son los nombres del programa (clase
        de ventana en KDE, ejecutable en Windows), en minúscula."""
        ids = [i.strip().lower() for i in ids if i and i.strip()]
        self.active_app = ids[0] if ids else ""
        self._active_ids = ids
        self._apply_app_layer()

    def _layer_for_active_app(self) -> int:
        layers = [{a.strip().lower() for a in layer.get("apps", []) if a.strip()} for layer in self.config["layers"]]
        # Primero los sitios (más específicos que el navegador), después los programas.
        site = self._browser_site
        if site:
            for i, apps in enumerate(layers):
                if any(site == a or site.endswith("." + a) for a in apps):
                    return i
        active = set(self._active_ids)
        for i, apps in enumerate(layers):
            if active & apps:
                return i
        return -1

    # ---------------- extensión del navegador ----------------

    def _on_browser_state(self, data: dict) -> None:
        site = str(data.get("site") or "").lower().removeprefix("www.")
        if site != self._browser_site:
            self._browser_site = site
            self._apply_app_layer()
        media = data.get("media")
        if isinstance(media, dict) and media.get("title"):
            self._browser_media, self._browser_media_at = media, time.monotonic()
        elif "media" in data:
            self._browser_media = None
        if "meeting" not in data:
            return
        meeting = data["meeting"] if isinstance(data["meeting"], dict) else {}
        meeting = {"active": bool(meeting), "mic": bool(meeting.get("mic")), "cam": bool(meeting.get("cam"))}
        if meeting != self._meeting:
            self._meeting = meeting
            self.link.send({"type": "meet", **meeting})

    @property
    def browser_site(self) -> str:
        return self._browser_site

    def browser_media_summary(self) -> dict | None:
        """Lo último que mandó la extensión sobre multimedia (para diagnóstico)."""
        bm = self._browser_media
        if not bm:
            return None
        return {"title": bm.get("title", ""), "artist": bm.get("artist", ""), "artwork": bm.get("artwork", ""),
                "age_s": round(time.monotonic() - self._browser_media_at, 1), "art_id": self._art_id}

    def _merge_browser_media(self, info: dict | None) -> dict | None:
        """Si lo que suena es el navegador, los datos de la extensión son mejores
        (título limpio, canal, tapa, posición)."""
        bm = self._browser_media
        if not bm or time.monotonic() - self._browser_media_at > BROWSER_MEDIA_MAX_AGE_S:
            return info
        if info is not None and not any(p in info.get("player", "") for p in BROWSER_PLAYERS):
            return info  # suena otro reproductor (Spotify, VLC...)
        if info is not None and not _same_title(info.get("title", ""), str(bm.get("title", ""))):
            return info  # la extensión habla de otra pestaña: mejor sin tapa que con la equivocada
        merged = dict(info or {})
        merged.update(
            playing=bool(bm.get("playing", merged.get("playing", False))),
            title=str(bm.get("title", "")), artist=str(bm.get("artist", "")), album=str(bm.get("album", "")),
            pos=int(bm.get("pos", merged.get("pos", 0)) or 0), len=int(bm.get("len", merged.get("len", 0)) or 0),
            art_url=str(bm.get("artwork") or merged.get("art_url", "")),
        )
        return merged

    def _apply_app_layer(self) -> None:
        match = self._layer_for_active_app()
        if match == self._app_match:
            return  # misma situación (ej: un diálogo del mismo programa): respetar lo que eligió el usuario
        self._app_match = match
        if match >= 0:
            if not self._auto_layer:
                self._manual_layer = self.layer_index
                self._auto_layer = True
            target = match
        elif self._auto_layer:
            self._auto_layer = False
            target = self._manual_layer
        else:
            return
        if target != self.layer_index:
            self.layer_index = target
            self._show_layer_change()

    def _show_layer_change(self) -> None:
        # Muestra la capa en la placa unos segundos, como si se hubiera tocado un botón.
        was_layer = self.mode == "layer"
        self._mark_activity()
        if was_layer:
            self._send_layer_snapshot()

    def _layer_chosen_by_user(self) -> None:
        # Cambio de capa a mano: pasa a ser la capa "de base" y ya no se pisa al salir del programa.
        self._auto_layer = False
        self._manual_layer = self.layer_index

    def _sync_device(self) -> None:
        self.link.send(settings_message(self.config))
        self.link.send(keymap_message(self.config))

    def select_layer(self, index: int) -> None:
        self.layer_index = max(0, min(index, len(self.config["layers"]) - 1))
        self._layer_chosen_by_user()
        self._mark_activity()
        self._send_layer_snapshot()

    def _on_device_event(self, evt: dict) -> None:
        etype = evt.get("type")
        if etype == "hello":
            # El firmware acaba de (re)bootear: resincronizamos todo.
            self.layer_index = 0
            self._layer_chosen_by_user()
            self._app_match = -1
            self._sync_device()
            self.link.send({"type": "view", "value": self.view_override or "auto"})
            self._last_media_msg = None  # la placa perdió lo que sonaba y la tapa: reenviar
            if self._meeting:
                self.link.send({"type": "meet", **self._meeting})
            if self._art_data:
                self._queue_art(self._art_id, self._art_data)
            self._enter_idle(force=True)
            self._apply_app_layer()
        elif etype == "art_ack":
            self._on_art_ack(evt)
        elif etype == "net":
            self.device_net = {k: evt.get(k) for k in ("wifi", "ip", "ble", "weather", "bat", "heap", "media")}
        elif etype == "button" and evt.get("state") == "down":
            if self._button_kind(evt.get("id", -1)) == "idle_view":
                # Cambia la pantalla de reposo: no tiene sentido mostrar la capa.
                self._quiet_until = time.monotonic() + VIEW_PRESS_QUIET_S
            else:
                self._mark_activity()
            self._run_button_action(evt.get("id", -1))
        elif etype == "encoder":
            if time.monotonic() >= self._quiet_until:
                self._mark_activity()
            self._run_encoder_action(evt.get("id", -1), evt.get("delta", 0))

    def _button_kind(self, button_id: int) -> str:
        buttons = self.current_layer.get("buttons", [])
        return buttons[button_id].get("action", "none") if 0 <= button_id < len(buttons) else "none"

    def _mark_activity(self) -> None:
        timeout_s = self.config.get("idle_timeout_sec", 15)
        self._activity_ticks_left = max(1, round(timeout_s * 1000 / STATS_PUSH_INTERVAL_MS))
        if self.mode != "layer":
            self._enter_layer()

    def _run_button_action(self, button_id: int) -> None:
        buttons = self.current_layer.get("buttons", [])
        if not (0 <= button_id < len(buttons)):
            return
        action = buttons[button_id]
        kind = action.get("action", "none")
        if kind == "key":
            self._send_keys(action.get("keys", []))
        elif kind == "command":
            run_command(action.get("cmd", ""))
        elif kind == "browser":
            self.browser.send_command({"action": action.get("cmd", "")})
        elif kind == "idle_view":
            i = VIEW_CYCLE.index(self.view_override)
            self.view_override = VIEW_CYCLE[(i + 1) % len(VIEW_CYCLE)]
            self.link.send({"type": "view", "value": self.view_override or "auto"})
            self._enter_idle()  # mostrar ya la pantalla elegida
        elif kind == "layer_next":
            self.layer_index = (self.layer_index + 1) % len(self.config["layers"])
            self._layer_chosen_by_user()
            self._send_layer_snapshot()
        elif kind == "layer_prev":
            self.layer_index = (self.layer_index - 1) % len(self.config["layers"])
            self._layer_chosen_by_user()
            self._send_layer_snapshot()
        elif kind == "layer_set":
            self.layer_index = max(0, min(action.get("index", 0), len(self.config["layers"]) - 1))
            self._layer_chosen_by_user()
            self._send_layer_snapshot()

    def _run_encoder_action(self, encoder_id: int, delta: int) -> None:
        encoders = self.current_layer.get("encoders", [])
        if not (0 <= encoder_id < len(encoders)):
            return
        mapping = encoders[encoder_id]
        if encoder_is_command(mapping):
            run_command(mapping.get("cw_cmd", "") if delta > 0 else mapping.get("ccw_cmd", ""))
            return
        if mapping.get("type") == "browser":
            self.browser.send_command({"action": mapping.get("cw_browser" if delta > 0 else "ccw_browser", "")})
            return
        keys = mapping.get("cw", []) if delta > 0 else mapping.get("ccw", [])
        self._send_keys(keys)

    def _send_keys(self, keys: list[str]) -> None:
        if not keys:
            return
        if layer_to_phone(self.current_layer):
            self.link.send({"type": "ble", "k": keys})
        else:
            self.injector.send_keys(keys)

    def _enter_layer(self) -> None:
        self.mode = "layer"
        self.mode_changed.emit(self.mode)
        self.link.send({"type": "mode", "value": "layer"})
        self._send_layer_snapshot()

    def _enter_idle(self, force: bool = False) -> None:
        if self.mode == "idle" and not force:
            return
        self.mode = "idle"
        self.mode_changed.emit(self.mode)
        self.link.send({"type": "mode", "value": "idle"})

    def _send_layer_snapshot(self) -> None:
        layer = self.current_layer
        labels = [action_label(a, self.view_override) for a in layer.get("buttons", [])]
        self.link.send(
            {"type": "layer", "index": self.layer_index, "name": layer.get("name", ""), "labels": labels,
             "bt": layer_to_phone(layer)}
        )
        self.layer_changed.emit(self.layer_index, layer.get("name", ""))

    # ---------------- multimedia ----------------

    def _update_media(self) -> None:
        info = self._merge_browser_media(self.media.read())
        url = info.get("art_url", "") if info else ""
        if url and not url.startswith(("file://", "https://")):
            url = ""  # solo archivos locales o https (nada de http plano ni esquemas raros)
        if url != self._art_url:
            self._art_url = url
            self._art_id, self._art_data = 0, None
            self._art_queue.clear()
            if url:
                self.art_loader.request(url)
        msg = {"type": "media", "active": info is not None, "art": self._art_id}
        if info:
            msg.update(playing=info["playing"], title=media_title(info["title"]), artist=ascii_text(info["artist"]),
                       album=ascii_text(info["album"]), pos=info["pos"], len=info["len"])
        if msg != self._last_media_msg:
            self._last_media_msg = msg
            self.link.send(msg)

    def _on_art_loaded(self, url: str, data) -> None:
        if url != self._art_url or not data:
            return
        self._art_counter += 1
        self._art_id, self._art_data = self._art_counter, data
        self._queue_art(self._art_id, data)
        self._last_media_msg = None  # mandar el id nuevo de la tapa en el próximo tick

    def _queue_art(self, art_id: int, data: bytes) -> None:
        self._art_queue = art_chunks(art_id, data)
        self._art_tries = 0
        self._send_art_chunk()

    def _send_art_chunk(self) -> None:
        """Manda (o reenvía) el primer pedazo pendiente y espera la confirmación."""
        if not self._art_queue or self._art_tries >= ART_MAX_TRIES:
            self._art_queue.clear()
            self._art_timer.stop()
            return
        self._art_tries += 1
        self.link.send(self._art_queue[0])
        self._art_timer.start()

    def _on_art_ack(self, evt: dict) -> None:
        if not self._art_queue:
            return
        head = self._art_queue[0]
        if evt.get("id") == head["id"] and evt.get("off") == head["off"] and evt.get("ok"):
            self._art_queue.pop(0)
            self._art_tries = 0
            if self._art_queue:
                self._send_art_chunk()
            else:
                self._art_timer.stop()

    def _tick(self) -> None:
        self._update_media()
        if self.mode == "idle":
            self.last_stats = self.stats.read()
            self.link.send({"type": "stats", **self.last_stats})
            return
        # modo "layer": contamos hacia el timeout de inactividad. El ping evita
        # que la placa crea que la PC se fue y pase al modo Bluetooth.
        self.link.send({"type": "ping"})
        self._activity_ticks_left -= 1
        if self._activity_ticks_left <= 0:
            self._enter_idle()

    def shutdown(self) -> None:
        self.injector.close()
