"""Inyección de teclas/atajos/multimedia vía evdev UInput.
Crea un teclado virtual a nivel kernel: funciona bajo Wayland sin X11
y sin necesitar ydotool, porque /dev/uinput ya tiene ACL de usuario
(systemd-logind uaccess) en esta sesión."""

import time

from evdev import UInput
from evdev import ecodes as e

_ALIASES = {
    "ctrl": "LEFTCTRL",
    "control": "LEFTCTRL",
    "rctrl": "RIGHTCTRL",
    "shift": "LEFTSHIFT",
    "rshift": "RIGHTSHIFT",
    "alt": "LEFTALT",
    "ralt": "RIGHTALT",
    "super": "LEFTMETA",
    "meta": "LEFTMETA",
    "win": "LEFTMETA",
    "vol+": "VOLUMEUP",
    "volup": "VOLUMEUP",
    "volumeup": "VOLUMEUP",
    "vol-": "VOLUMEDOWN",
    "voldown": "VOLUMEDOWN",
    "volumedown": "VOLUMEDOWN",
    "mute": "MUTE",
    "brightnessup": "BRIGHTNESSUP",
    "brightnessdown": "BRIGHTNESSDOWN",
    "play": "PLAYPAUSE",
    "playpause": "PLAYPAUSE",
    "pause": "PAUSE",
    "stop": "STOPCD",
    "next": "NEXTSONG",
    "nextsong": "NEXTSONG",
    "prev": "PREVIOUSSONG",
    "previoussong": "PREVIOUSSONG",
}


def _resolve(name: str) -> int:
    key = name.strip().lower()
    canon = _ALIASES.get(key, key.upper())
    code_name = canon if canon.startswith("KEY_") else f"KEY_{canon}"
    try:
        return e.ecodes[code_name]
    except KeyError as exc:
        raise ValueError(f"Tecla desconocida: {name!r}") from exc


class LinuxKeyInjector:
    def __init__(self) -> None:
        # KEY_CNT (768) es un marcador de límite, no una tecla real: supera
        # KEY_MAX (767) y uinput lo rechaza con EINVAL si se intenta registrar.
        keycodes = sorted(
            {code for name, code in e.ecodes.items() if name.startswith("KEY_") and code <= 767}
        )
        self._ui = UInput({e.EV_KEY: keycodes}, name="ttgo-macropad-virtual-kbd")

    def send_keys(self, key_names: list[str]) -> None:
        if not key_names:
            return
        try:
            codes = [_resolve(name) for name in key_names]
        except ValueError:
            return  # tecla mal escrita en la config: se ignora, no se rompe la app
        for code in codes:
            self._ui.write(e.EV_KEY, code, 1)
        self._ui.syn()
        time.sleep(0.01)
        for code in reversed(codes):
            self._ui.write(e.EV_KEY, code, 0)
        self._ui.syn()

    def close(self) -> None:
        self._ui.close()
