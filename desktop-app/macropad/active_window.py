"""Detecta el programa en primer plano para las capas por programa.

- Linux/KDE (Wayland): un programa común no puede ver la ventana activa, así
  que un script de KWin (`desktop-app/kwin-script/`) avisa por D-Bus cada vez
  que cambia, llamando a `activeWindow(clase, nombre)` en el servicio
  io.github.yyymira.TtgoMacropad. Sin ese script, simplemente no llega nada.
- Windows: se consulta la ventana en primer plano dos veces por segundo y se
  toma el nombre de su ejecutable.

En ambos casos se le pasa al Controller una lista de nombres posibles del
programa (ej: ["firefox", "firefox"] o ["firefox.exe", "firefox"])."""

import sys

from PySide6.QtCore import QObject, QTimer

DBUS_SERVICE = "io.github.yyymira.TtgoMacropad"
DBUS_PATH = "/ActiveWindow"
DBUS_INTERFACE = "io.github.yyymira.TtgoMacropad.ActiveWindow"


def start_active_window_watcher(controller) -> QObject | None:
    """Arranca el detector que corresponda; hay que guardar la referencia."""
    if sys.platform == "win32":
        return _WindowsPoller(controller)
    if sys.platform.startswith("linux"):
        return _make_dbus_receiver(controller)
    return None


def _make_dbus_receiver(controller) -> QObject | None:
    try:
        from PySide6.QtCore import ClassInfo, Slot
        from PySide6.QtDBus import QDBusConnection
    except ImportError:
        return None

    @ClassInfo({"D-Bus Interface": DBUS_INTERFACE})
    class Receiver(QObject):
        @Slot(str, str)
        def activeWindow(self, resource_class: str, resource_name: str) -> None:  # noqa: N802 (nombre D-Bus)
            controller.set_active_window([resource_class, resource_name])

    receiver = Receiver()
    bus = QDBusConnection.sessionBus()
    if not bus.isConnected():
        return None
    bus.registerService(DBUS_SERVICE)
    bus.registerObject(DBUS_PATH, receiver, QDBusConnection.RegisterOption.ExportAllSlots)
    return receiver


class _WindowsPoller(QObject):
    def __init__(self, controller):
        super().__init__()
        import ctypes
        from ctypes import wintypes

        import psutil

        self._controller = controller
        self._user32 = ctypes.windll.user32
        self._wintypes = wintypes
        self._ctypes = ctypes
        self._psutil = psutil
        self._last_pid = None
        self._timer = QTimer(self)
        self._timer.setInterval(500)
        self._timer.timeout.connect(self._poll)
        self._timer.start()

    def _poll(self) -> None:
        hwnd = self._user32.GetForegroundWindow()
        if not hwnd:
            return
        pid = self._wintypes.DWORD()
        self._user32.GetWindowThreadProcessId(hwnd, self._ctypes.byref(pid))
        if pid.value == self._last_pid:
            return
        self._last_pid = pid.value
        try:
            exe = self._psutil.Process(pid.value).name()
        except (self._psutil.Error, OSError):
            return
        stem = exe[:-4] if exe.lower().endswith(".exe") else exe
        self._controller.set_active_window([exe, stem])
