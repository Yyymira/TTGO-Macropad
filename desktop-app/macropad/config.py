"""Persistencia de la configuración: capas, mapeos de botones/encoders
y timeout de inactividad. Vive en ~/.config/ttgo-macropad/config.json."""

import copy
import json
from pathlib import Path

CONFIG_PATH = Path.home() / ".config" / "ttgo-macropad" / "config.json"

NUM_BUTTONS = 6
NUM_ENCODERS = 2

DEFAULT_CONFIG = {
    "idle_timeout_sec": 15,
    "layers": [
        {
            "name": "General",
            "buttons": [
                {"action": "key", "keys": ["playpause"]},
                {"action": "key", "keys": ["nextsong"]},
                {"action": "key", "keys": ["previoussong"]},
                {"action": "key", "keys": ["mute"]},
                {"action": "layer_next"},
                {"action": "none"},
            ],
            "encoders": [
                {"cw": ["volumeup"], "ccw": ["volumedown"]},
                {"cw": ["brightnessup"], "ccw": ["brightnessdown"]},
            ],
        },
        {
            "name": "Navegador",
            "buttons": [
                {"action": "key", "keys": ["ctrl", "t"]},
                {"action": "key", "keys": ["ctrl", "w"]},
                {"action": "key", "keys": ["ctrl", "shift", "t"]},
                {"action": "key", "keys": ["ctrl", "tab"]},
                {"action": "layer_next"},
                {"action": "none"},
            ],
            "encoders": [
                {"cw": ["volumeup"], "ccw": ["volumedown"]},
                {"cw": ["brightnessup"], "ccw": ["brightnessdown"]},
            ],
        },
    ],
}


def load_config() -> dict:
    if CONFIG_PATH.exists():
        try:
            data = json.loads(CONFIG_PATH.read_text())
            if data.get("layers"):
                return data
        except (json.JSONDecodeError, OSError):
            pass
    return copy.deepcopy(DEFAULT_CONFIG)


def save_config(config: dict) -> None:
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    CONFIG_PATH.write_text(json.dumps(config, indent=2, ensure_ascii=False))
