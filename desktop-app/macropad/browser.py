"""Puente con la extensión del navegador (`browser-extension/`).

La extensión le habla al servicio por la API local (127.0.0.1:47823):
- POST /browser/pair   → pide la clave; solo funciona mientras el widget tiene
                          abierta la ventana de vinculación (botón "Vincular").
- POST /browser/state  → qué sitio está al frente, qué suena, estado de la reunión.
- GET  /browser/poll   → espera órdenes (long-poll de hasta POLL_TIMEOUT_S).
Todas menos /browser/pair llevan la clave en el header X-Macropad-Key. Las
páginas web no la conocen, y sin ella no pueden mandar nada.

Las órdenes salen de los botones/encoders con la acción "Navegador" y las
ejecuta la extensión en la pestaña que corresponda."""

import os
import secrets
import threading
import time
from pathlib import Path

from PySide6.QtCore import QObject, Signal

KEY_PATH = Path.home() / ".config" / "ttgo-macropad" / "browser_key"
PAIRING_WINDOW_S = 120
POLL_TIMEOUT_S = 20
SEEN_TIMEOUT_S = POLL_TIMEOUT_S + 15  # sin noticias de la extensión por más que esto = desconectada

# Acciones que entiende la extensión (las mismas en el widget y en browser-extension/background.js)
ACTIONS = [
    "speed_up", "speed_down", "speed_reset", "seek_forward", "seek_back", "skip_ad",
    "tab_volume_up", "tab_volume_down", "focus_media_tab", "toggle_mic", "toggle_cam",
]


def _load_or_create_key() -> str:
    try:
        key = KEY_PATH.read_text().strip()
        if key:
            return key
    except OSError:
        pass
    key = secrets.token_urlsafe(24)
    KEY_PATH.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(KEY_PATH, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w") as f:
        f.write(key)
    return key


class BrowserLink(QObject):
    """Lo usan dos hilos: el de HTTP (state/poll/pair) y el de Qt (controller)."""

    state_received = Signal(dict)  # se emite desde el hilo HTTP; Qt lo encola al Controller

    def __init__(self) -> None:
        super().__init__()
        self.key = _load_or_create_key()
        self._cond = threading.Condition()
        self._commands: list[dict] = []
        self._pairing_until = 0.0
        self._last_seen = 0.0

    # --- vinculación ---
    def open_pairing(self) -> None:
        self._pairing_until = time.monotonic() + PAIRING_WINDOW_S

    def pairing_open(self) -> bool:
        return time.monotonic() < self._pairing_until

    def pair(self) -> str | None:
        if not self.pairing_open():
            return None
        self._pairing_until = 0.0  # una sola extensión por apertura
        return self.key

    def check_key(self, sent: str) -> bool:
        ok = secrets.compare_digest(sent or "", self.key)
        if ok:
            self._last_seen = time.monotonic()
        return ok

    def connected(self) -> bool:
        return time.monotonic() - self._last_seen < SEEN_TIMEOUT_S

    # --- órdenes (Qt → extensión) ---
    def send_command(self, command: dict) -> None:
        if not self.connected():
            return  # sin extensión: no guardar órdenes viejas para cuando aparezca
        with self._cond:
            self._commands.append(command)
            del self._commands[:-20]  # si la extensión no está, no acumular de más
            self._cond.notify_all()

    def poll(self) -> list[dict]:
        with self._cond:
            if not self._commands:
                self._cond.wait(POLL_TIMEOUT_S)
            out, self._commands = self._commands, []
        return out
