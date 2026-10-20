"""Elige el inyector de teclas según el sistema operativo: evdev/uinput en
Linux (teclado virtual a nivel kernel, sin X11/ydotool) o la API Win32 en
Windows (keybd_event + WMI para brillo). La API pública (`KeyInjector`,
`send_keys`, `close`) es la misma en ambos — `controller.py` no necesita
saber cuál está corriendo."""

import sys

if sys.platform == "win32":
    from macropad.injector_windows import WindowsKeyInjector as KeyInjector
elif sys.platform.startswith("linux"):
    from macropad.injector_linux import LinuxKeyInjector as KeyInjector
else:
    raise RuntimeError(f"Plataforma no soportada: {sys.platform}")
