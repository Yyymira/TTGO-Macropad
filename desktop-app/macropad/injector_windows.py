"""Inyección de teclas/atajos/multimedia en Windows.

No existe un equivalente directo a evdev/uinput en Windows, así que acá se
usa la API Win32 clásica (`keybd_event`) para teclas, combos, volumen y
multimedia — son eventos de teclado reales a nivel de sistema, igual que en
Linux, y disparan el OSD nativo de Windows para volumen.

El brillo de pantalla es un caso especial: Windows no tiene una tecla
virtual estándar para subir/bajar brillo (es algo propio del firmware de
cada teclado), así que se controla vía WMI (`WmiMonitorBrightnessMethods`),
que solo funciona en paneles que lo soportan (típicamente notebooks). Si el
paquete opcional `wmi` no está instalado o el monitor no lo soporta, esas
dos acciones simplemente no hacen nada — no rompe el resto de la app."""

import ctypes
import time

user32 = ctypes.windll.user32

KEYEVENTF_EXTENDEDKEY = 0x0001
KEYEVENTF_KEYUP = 0x0002

# Teclas "extendidas" (volumen/multimedia): necesitan el flag EXTENDEDKEY
# para que Windows las trate como las teclas dedicadas del teclado.
_EXTENDED = {0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3}

_VK = {
    "ctrl": 0x11, "control": 0x11, "lctrl": 0xA2, "rctrl": 0xA3,
    "shift": 0x10, "lshift": 0xA0, "rshift": 0xA1,
    "alt": 0x12, "lalt": 0xA4, "ralt": 0xA5,
    "super": 0x5B, "meta": 0x5B, "win": 0x5B,
    "tab": 0x09, "esc": 0x1B, "escape": 0x1B, "enter": 0x0D, "return": 0x0D,
    "space": 0x20, "backspace": 0x08, "delete": 0x2E, "del": 0x2E,
    "up": 0x26, "down": 0x28, "left": 0x25, "right": 0x27,
    "home": 0x24, "end": 0x23, "pageup": 0x21, "pagedown": 0x22,
    "vol+": 0xAF, "volup": 0xAF, "volumeup": 0xAF,
    "vol-": 0xAE, "voldown": 0xAE, "volumedown": 0xAE,
    "mute": 0xAD,
    "play": 0xB3, "playpause": 0xB3, "pause": 0xB3,
    "stop": 0xB2,
    "next": 0xB0, "nextsong": 0xB0,
    "prev": 0xB1, "previoussong": 0xB1,
}
for _i in range(1, 13):
    _VK[f"f{_i}"] = 0x6F + _i  # F1=0x70 ... F12=0x7B


def _resolve(name: str) -> int:
    key = name.strip().lower()
    if key in _VK:
        return _VK[key]
    if len(key) == 1 and key.isalnum():
        return ord(key.upper())  # letras/dígitos: el VK code coincide con el ASCII en mayúscula
    raise ValueError(f"Tecla desconocida: {name!r}")


class WindowsKeyInjector:
    def __init__(self) -> None:
        pass  # keybd_event es una llamada directa a la API, no hay nada que inicializar

    def send_keys(self, key_names: list[str]) -> None:
        if not key_names:
            return
        low = [name.strip().lower() for name in key_names]
        if len(low) == 1 and low[0] in ("brightnessup", "brightnessdown"):
            self._adjust_brightness(low[0])
            return
        try:
            codes = [_resolve(name) for name in low]
        except ValueError:
            return  # tecla mal escrita en la config: se ignora, no se rompe la app
        for code in codes:
            flags = KEYEVENTF_EXTENDEDKEY if code in _EXTENDED else 0
            user32.keybd_event(code, 0, flags, 0)
        time.sleep(0.01)
        for code in reversed(codes):
            flags = KEYEVENTF_EXTENDEDKEY if code in _EXTENDED else 0
            user32.keybd_event(code, 0, flags | KEYEVENTF_KEYUP, 0)

    @staticmethod
    def _adjust_brightness(direction: str) -> None:
        try:
            import wmi  # paquete opcional: requirements-windows.txt

            conn = wmi.WMI(namespace="root\\WMI")
            current = conn.WmiMonitorBrightness()[0].CurrentBrightness
            step = 10 if direction == "brightnessup" else -10
            target = max(0, min(100, current + step))
            for method in conn.WmiMonitorBrightnessMethods():
                method.WmiSetBrightness(target, 0)
        except Exception:
            pass  # sin paquete `wmi` o panel sin soporte WMI: se ignora

    def close(self) -> None:
        pass  # no hay ningún recurso que liberar (a diferencia del uinput de Linux)
