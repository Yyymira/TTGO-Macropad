"""Cerebro de la app: máquina de estados de capas, timer de inactividad,
despacho de eventos de botones/encoders a acciones, y empuje continuo de
stats del sistema a la pantalla idle de la placa."""

import time

from PySide6.QtCore import QObject, Signal, QTimer

from macropad.config import load_config, save_config
from macropad.injector import KeyInjector
from macropad.serial_link import SerialLink
from macropad.stats import StatsReader

STATS_PUSH_INTERVAL_MS = 1000


def action_label(action: dict) -> str:
    kind = action.get("action", "none")
    if kind == "key":
        return "+".join(k.upper() for k in action.get("keys", []))[:14]
    if kind == "layer_next":
        return "Capa >>"
    if kind == "layer_prev":
        return "<< Capa"
    if kind == "layer_set":
        return f"Capa {action.get('index', 0)}"
    return ""


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
            "e": [{"cw": e.get("cw", []), "ccw": e.get("ccw", [])} for e in layer.get("encoders", [])],
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
        self._sync_device()
        if self.mode == "layer":
            self._send_layer_snapshot()

    def _sync_device(self) -> None:
        self.link.send(settings_message(self.config))
        self.link.send(keymap_message(self.config))

    def select_layer(self, index: int) -> None:
        self.layer_index = max(0, min(index, len(self.config["layers"]) - 1))
        self._mark_activity()
        self._send_layer_snapshot()

    def _on_device_event(self, evt: dict) -> None:
        etype = evt.get("type")
        if etype == "hello":
            # El firmware acaba de (re)bootear: resincronizamos todo.
            self.layer_index = 0
            self._sync_device()
            self._enter_idle(force=True)
        elif etype == "net":
            self.device_net = {k: evt.get(k) for k in ("wifi", "ip", "ble", "weather")}
        elif etype == "button" and evt.get("state") == "down":
            self._mark_activity()
            self._run_button_action(evt.get("id", -1))
        elif etype == "encoder":
            self._mark_activity()
            self._run_encoder_action(evt.get("id", -1), evt.get("delta", 0))

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
            self.injector.send_keys(action.get("keys", []))
        elif kind == "layer_next":
            self.layer_index = (self.layer_index + 1) % len(self.config["layers"])
            self._send_layer_snapshot()
        elif kind == "layer_prev":
            self.layer_index = (self.layer_index - 1) % len(self.config["layers"])
            self._send_layer_snapshot()
        elif kind == "layer_set":
            self.layer_index = max(0, min(action.get("index", 0), len(self.config["layers"]) - 1))
            self._send_layer_snapshot()

    def _run_encoder_action(self, encoder_id: int, delta: int) -> None:
        encoders = self.current_layer.get("encoders", [])
        if not (0 <= encoder_id < len(encoders)):
            return
        mapping = encoders[encoder_id]
        keys = mapping.get("cw", []) if delta > 0 else mapping.get("ccw", [])
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
        labels = [action_label(a) for a in layer.get("buttons", [])]
        self.link.send(
            {"type": "layer", "index": self.layer_index, "name": layer.get("name", ""), "labels": labels}
        )
        self.layer_changed.emit(self.layer_index, layer.get("name", ""))

    def _tick(self) -> None:
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
