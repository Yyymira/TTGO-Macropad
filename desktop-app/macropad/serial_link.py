"""Conexión serie con la placa TTGO: detecta el puerto, reconecta solo,
lee líneas JSON del device y permite mandarle comandos."""

import json
import threading
import time

import serial
from PySide6.QtCore import QObject, Signal
from serial.tools import list_ports


class SerialLink(QObject):
    event_received = Signal(dict)
    connected_changed = Signal(bool)

    def __init__(self, port: str | None = None, baud: int = 115200):
        super().__init__()
        self._port = port
        self._baud = baud
        self._ser: serial.Serial | None = None
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._connected = False

    def start(self) -> None:
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:
                pass

    def send(self, obj: dict) -> None:
        if not self._ser or not self._connected:
            return
        try:
            self._ser.write((json.dumps(obj) + "\n").encode())
        except (serial.SerialException, OSError):
            self._set_connected(False)

    @property
    def connected(self) -> bool:
        return self._connected

    def _set_connected(self, value: bool) -> None:
        if value != self._connected:
            self._connected = value
            self.connected_changed.emit(value)

    def _find_port(self) -> str | None:
        if self._port:
            return self._port
        # list_ports enumera /dev/ttyACM*, /dev/ttyUSB* en Linux y COM* en
        # Windows por igual. Se filtra por VID USB para descartar los puertos
        # serie legacy de la placa madre (/dev/ttyS*, COM1), que siempre existen.
        candidates = sorted(p.device for p in list_ports.comports() if p.vid is not None)
        return candidates[0] if candidates else None

    def _run(self) -> None:
        while not self._stop.is_set():
            port = self._find_port()
            if not port:
                time.sleep(1)
                continue
            try:
                self._ser = serial.Serial(port, self._baud, timeout=0.5)
                self._set_connected(True)
                self._read_loop()
            except (serial.SerialException, OSError):
                self._set_connected(False)
                time.sleep(1)

    def _read_loop(self) -> None:
        buf = b""
        while not self._stop.is_set():
            try:
                chunk = self._ser.read(256)
            except (serial.SerialException, OSError):
                break
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                line = line.strip()
                if not line:
                    continue
                try:
                    obj = json.loads(line.decode(errors="replace"))
                except json.JSONDecodeError:
                    continue
                self.event_received.emit(obj)
        self._set_connected(False)
