"""Persistencia de la configuración: capas, mapeos de botones/encoders
y timeout de inactividad. Vive en ~/.config/ttgo-macropad/config.json."""

import copy
import json
import os
import secrets
from pathlib import Path

from macropad.i18n import tr

CONFIG_PATH = Path.home() / ".config" / "ttgo-macropad" / "config.json"

NUM_BUTTONS = 10  # 0-5: Cherry MX (grilla 3x2), 6-7: integrados de la placa, 8-9: pulsador de los encoders
NUM_ENCODERS = 2

DEFAULT_CONFIG = {
    "idle_timeout_sec": 15,
    "layers": [
        {
            "name": "General",
            "buttons": [
                {"action": "key", "keys": ["previoussong"]},
                {"action": "key", "keys": ["playpause"]},
                {"action": "key", "keys": ["nextsong"]},
                {"action": "key", "keys": ["mute"]},
                {"action": "none"},
                {"action": "none"},
                {"action": "layer_prev"},
                {"action": "layer_next"},
                {"action": "none"},
                {"action": "none"},
            ],
            "encoders": [
                {"cw": ["volumeup"], "ccw": ["volumedown"]},
                {"cw": ["brightnessup"], "ccw": ["brightnessdown"]},
            ],
        },
        {
            "name": tr("Browser"),
            "buttons": [
                {"action": "key", "keys": ["ctrl", "t"]},
                {"action": "key", "keys": ["ctrl", "w"]},
                {"action": "key", "keys": ["ctrl", "shift", "t"]},
                {"action": "key", "keys": ["ctrl", "tab"]},
                {"action": "key", "keys": ["ctrl", "shift", "tab"]},
                {"action": "key", "keys": ["f5"]},
                {"action": "layer_prev"},
                {"action": "layer_next"},
                {"action": "none"},
                {"action": "none"},
            ],
            "encoders": [
                {"cw": ["volumeup"], "ccw": ["volumedown"]},
                {"cw": ["brightnessup"], "ccw": ["brightnessdown"]},
            ],
        },
    ],
}


# Ajustes que se copian a la placa (WiFi, clima, pantalla de reposo, OTA).
DEFAULT_DEVICE = {
    "idle_view": "both",  # "stats" | "weather" | "both"
    "stats_secs": 10,  # modo "both": cuánto dura cada pantalla
    "weather_secs": 10,
    "wifi_ssid": "",
    "wifi_pass": "",
    "city": "",
    "place": "",  # nombre completo que devolvió el buscador de ciudades
    "lat": None,
    "lon": None,
    "ota_pass": "",
}


def _with_device_defaults(config: dict) -> tuple[dict, bool]:
    device = {**DEFAULT_DEVICE, **config.get("device", {})}
    generated = False
    if not device["ota_pass"]:
        device["ota_pass"] = secrets.token_urlsafe(12)
        generated = True
    config["device"] = device
    return config, generated


def load_config() -> dict:
    config = None
    if CONFIG_PATH.exists():
        try:
            data = json.loads(CONFIG_PATH.read_text())
            if data.get("layers"):
                config = data
        except (json.JSONDecodeError, OSError):
            pass
    if config is None:
        config = copy.deepcopy(DEFAULT_CONFIG)
    config, generated = _with_device_defaults(config)
    if generated:
        save_config(config)  # la contraseña OTA tiene que ser estable entre arranques
    return config


def save_config(config: dict) -> None:
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    # Incluye la contraseña del WiFi y la de OTA: solo legible por el usuario.
    fd = os.open(CONFIG_PATH, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w") as f:
        f.write(json.dumps(config, indent=2, ensure_ascii=False))
    os.chmod(CONFIG_PATH, 0o600)
