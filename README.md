# TTGO Macropad

Mini teclado de macros con 6 botones + 2 encoders rotativos sobre una placa
**TTGO T-Display** (ESP32 + pantalla ST7789 1.14"), programable desde una app
de escritorio en Python (Linux/Wayland/KDE).

- En reposo, la pantalla muestra gauges tipo velocímetro para CPU% y GPU%
  (GPU vía `nvidia-smi`), una barra para RAM% y números para las
  temperaturas de CPU y GPU — todo se actualiza solo, sin tocar nada.
- Al tocar un botón o encoder, pasa a mostrar la capa activa (grilla con las
  6 acciones). Tras unos segundos de inactividad (configurable), vuelve sola
  a la pantalla de stats.
- Un botón de cada capa puede estar mapeado a "capa siguiente/anterior/ir a
  capa N", permitiendo tener varios set de atajos.
- Los encoders inyectan teclas reales (volumen, brillo, o lo que el usuario
  quiera) vía un teclado virtual a nivel kernel (`evdev`/`uinput`), sin
  necesitar X11 ni ydotool.

Ver [docs/schematic.html](docs/schematic.html) (esquemático visual) o
[docs/wiring.md](docs/wiring.md) para el pinout y la lista de compras —
**todavía falta cablear 4 botones + 2 encoders** (los 2 botones integrados de
la placa ya funcionan).

## Arquitectura

La placa ESP32 de este modelo no tiene USB nativo (confirmado con
`esptool chip-id`: es un ESP32-D0WDQ6-V3 con puente serie CH9102), así que no
puede ser un teclado HID por sí misma. Por eso la placa es un periférico
"tonto" — lee botones/encoders y dibuja la pantalla — y toda la lógica
(capas, mapeos, qué mostrar) vive en la app de escritorio, que le habla por
un protocolo JSON-por-línea sobre serial USB a 115200 baud. El detalle del
protocolo está en los comentarios de `firmware/src/protocol.cpp` y
`desktop-app/macropad/serial_link.py`.

## Firmware (`firmware/`)

Hecho con PlatformIO (Arduino framework, board `esp32dev`), usando
`TFT_eSPI` para la pantalla, `Bounce2` para los botones y `ESP32Encoder`
para los encoders.

```bash
cd firmware
python3 -m venv .venv          # si no existe todavía
.venv/bin/pip install platformio
.venv/bin/pio run                              # compilar
.venv/bin/pio run -t upload --upload-port /dev/ttyACM0   # flashear
.venv/bin/pio device monitor -p /dev/ttyACM0 -b 115200    # ver logs/eventos
```

## App de escritorio (`desktop-app/`)

Python + PySide6. Usa `psutil` (stats), `evdev` (inyección de teclas) y
`pyserial` (comunicación con la placa). En Fedora, `psutil`/`evdev`/`PySide6`
ya vienen como paquetes de sistema (`python3-psutil`, `python3-evdev`,
`python3-pyside6`), por eso el venv se crea con `--system-site-packages` y
solo instala `pyserial` aparte.

```bash
cd desktop-app
python3 -m venv --system-site-packages .venv   # si no existe todavía
.venv/bin/pip install -r requirements.txt
.venv/bin/python main.py
```

La ventana tiene dos pestañas:

- **Estado**: conexión con la placa, modo actual (idle/capa) y un log de
  eventos crudos (útil para debug).
- **Configuración**: editor de capas — por cada capa se define la acción de
  los 6 botones y de los 2 encoders, y el timeout de inactividad. "Guardar y
  aplicar" persiste en `~/.config/ttgo-macropad/config.json` y lo usa al
  toque, sin reiniciar la app ni la placa.

Cerrar la ventana la manda a la bandeja del sistema (sigue corriendo); para
salir de verdad, usar "Salir" del menú del ícono de bandeja.

### Acciones de botón disponibles

- `Ninguna`
- `Tecla / atajo`: lista de teclas separadas por coma (ej: `ctrl,t` o
  `playpause`). Nombres soportados: cualquier `KEY_*` de evdev (en
  minúscula, sin el prefijo), más los alias cómodos definidos en
  `macropad/injector.py` (`ctrl`, `shift`, `alt`, `super`, `vol+`, `vol-`,
  `mute`, `play`, `next`, `prev`, `brightnessup`, `brightnessdown`, etc).
- `Capa siguiente` / `Capa anterior` / `Ir a capa N`

### Acciones de encoder

Cada encoder tiene una lista de teclas para "horario" y otra para
"antihorario" (mismo formato que arriba). Los presets típicos son
`volumeup`/`volumedown` y `brightnessup`/`brightnessdown`, pero se puede
poner cualquier cosa (zoom, scroll, lo que sea).

## Estado del proyecto

- [x] Firmware base: lectura de botones/encoders, display con pantalla idle
      y pantalla de capa, protocolo JSON sobre serial.
- [x] App: conexión serial con reconexión automática, stats en vivo,
      máquina de estados de capas con timeout de inactividad, inyección de
      teclas vía `evdev`, editor de capas/mapeos con persistencia, ícono de
      bandeja.
- [x] Probado de punta a punta con los 2 botones integrados de la placa
      (play/pause real sobre un video de YouTube, cambio de capa, vuelta
      automática a stats).
- [ ] Cablear los 4 botones y 2 encoders restantes (ver
      [docs/wiring.md](docs/wiring.md)) y probar esa parte.
- [ ] Opcional: autostart de la app al iniciar sesión (sería un servicio de
      usuario systemd apuntando a `desktop-app/.venv/bin/python main.py`).
