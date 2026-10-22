"""Lo que suena en la PC, para la pantalla de multimedia de la placa.

En Linux se lee por MPRIS (D-Bus), el mismo estándar que usa el widget de
multimedia de Plasma: Spotify, navegadores, VLC, Elisa, etc. En Windows
todavía no hay soporte (read() devuelve None y la pantalla no aparece).

La tapa se procesa acá y no en la placa: se abre, se recorta al centro, se
achica a ART_SIZE x ART_SIZE y se convierte a RGB565, el formato de la
pantalla. Los textos se pasan a ASCII porque las fuentes de la placa no
tienen tildes ni otros alfabetos."""

import base64
import re
import threading
import unicodedata
import urllib.request
from pathlib import Path
from urllib.parse import unquote, urlparse

from PySide6.QtCore import QObject, Qt, Signal
from PySide6.QtGui import QImage

ART_SIZE = 96  # igual que en firmware/include/media.h


ART_CHUNK = 2400  # bytes por mensaje "art" (3200 en base64: entra en una línea del protocolo)


def media_title(text: str) -> str:
    # Los navegadores anteponen el contador de notificaciones: "(38) Tema - YouTube"
    return ascii_text(re.sub(r"^\(\d+\)\s*", "", text or ""))


def art_chunks(art_id: int, data: bytes) -> list[dict]:
    return [
        {"type": "art", "id": art_id, "off": off, "d": base64.b64encode(data[off:off + ART_CHUNK]).decode()}
        for off in range(0, len(data), ART_CHUNK)
    ]


def ascii_text(text: str) -> str:
    text = unicodedata.normalize("NFKD", text or "")
    text = "".join(c for c in text if not unicodedata.combining(c))
    text = text.encode("ascii", "ignore").decode()
    return " ".join(text.split())


class MediaReader:
    """Devuelve el reproductor más relevante: el que está sonando, o si no
    hay ninguno, uno en pausa."""

    def __init__(self) -> None:
        try:
            import dbus  # dbus-python, paquete del sistema (python3-dbus)

            self._dbus = dbus
            self._bus = dbus.SessionBus()
        except Exception:
            self._bus = None

    def read(self) -> dict | None:
        if self._bus is None:
            return None
        best = None
        try:
            names = [n for n in self._bus.list_names() if str(n).startswith("org.mpris.MediaPlayer2.")]
        except Exception:
            return None
        for name in names:
            try:
                obj = self._bus.get_object(name, "/org/mpris/MediaPlayer2")
                props = self._dbus.Interface(obj, "org.freedesktop.DBus.Properties")
                p = props.GetAll("org.mpris.MediaPlayer2.Player")
            except Exception:
                continue
            status = str(p.get("PlaybackStatus", "Stopped"))
            if status == "Stopped":
                continue
            md = p.get("Metadata", {})
            artists = md.get("xesam:artist", [])
            info = {
                "playing": status == "Playing",
                "title": str(md.get("xesam:title", "")),
                "artist": ", ".join(str(a) for a in artists if str(a)),
                "album": str(md.get("xesam:album", "")),
                "pos": int(p.get("Position", 0)) // 1_000_000,
                "len": int(md.get("mpris:length", 0)) // 1_000_000,
                "art_url": str(md.get("mpris:artUrl", "")),
                "player": str(name).lower(),
            }
            if info["playing"]:
                return info
            best = best or info
        return best


def art_to_rgb565(url: str) -> bytes | None:
    """Abre la tapa (file:// o http/https) y la devuelve como RGB565 crudo."""
    try:
        parsed = urlparse(url)
        if parsed.scheme == "file":
            data = Path(unquote(parsed.path)).read_bytes()
        elif parsed.scheme in ("http", "https"):
            with urllib.request.urlopen(url, timeout=5) as resp:  # noqa: S310 (URL del reproductor)
                data = resp.read(5_000_000)
        else:
            return None
    except Exception:
        return None
    img = QImage.fromData(data)
    if img.isNull():
        return None
    img = img.scaled(ART_SIZE, ART_SIZE, Qt.AspectRatioMode.KeepAspectRatioByExpanding,
                     Qt.TransformationMode.SmoothTransformation)
    x = (img.width() - ART_SIZE) // 2
    y = (img.height() - ART_SIZE) // 2
    img = img.copy(x, y, ART_SIZE, ART_SIZE).convertToFormat(QImage.Format.Format_RGB16)
    out = bytearray()
    for row in range(ART_SIZE):  # sin el relleno de fin de línea que pueda tener QImage
        out += bytes(img.constScanLine(row))[: ART_SIZE * 2]
    return bytes(out)


class ArtLoader(QObject):
    """Descarga y convierte la tapa en otro hilo (puede ser una URL lenta)."""

    loaded = Signal(str, object)  # url, bytes RGB565 o None

    def __init__(self) -> None:
        super().__init__()
        self._cache: dict[str, bytes | None] = {}

    def request(self, url: str) -> None:
        if url in self._cache:
            self.loaded.emit(url, self._cache[url])
            return

        def work() -> None:
            data = art_to_rgb565(url)
            if len(self._cache) > 20:
                self._cache.clear()
            self._cache[url] = data
            self.loaded.emit(url, data)

        threading.Thread(target=work, daemon=True).start()
