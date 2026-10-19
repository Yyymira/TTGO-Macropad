# Cableado — TTGO T-Display (ESP32)

Placa confirmada por sondeo directo (`esptool chip-id`): **ESP32-D0WDQ6-V3**, sin USB
nativo (puente serie CH9102), display ST7789 1.14" 240x135.

## Lista de compras

- 4x pulsador táctil momentáneo (4 pines, tipo 6x6mm) — los botones 0 y 1 ya están
  integrados en la placa, no hace falta comprarlos.
- 2x encoder rotativo EC11 (solo se usan las 2 señales de cuadratura CLK/DT; el
  push-button del eje, si el modelo lo trae, queda sin cablear por ahora).
- Cable dupont hembra-hembra o protoboard, según cómo lo vayas a montar.

## Pines usados por la placa (no tocar)

| Función | GPIO |
|---|---|
| Display MOSI | 19 |
| Display SCK | 18 |
| Display CS | 5 |
| Display DC | 16 |
| Display RST | 23 |
| Display BL (backlight) | 4 |
| Batería (ADC) | 34 |
| USB-Serial (UART0 TX/RX) | 1 / 3 |

## Pinout para el macropad

| Función | GPIO | Notas |
|---|---|---|
| Botón 0 | 0 | Integrado en la placa. Ojo: si lo mantenés apretado al conectar el USB o resetear, la placa entra en modo de flasheo en vez de arrancar normal. En uso normal (ya booteada) funciona como botón cualquiera. |
| Botón 1 | 35 | Integrado en la placa. |
| Botón 2 | 32 | A cablear: pulsador entre GPIO32 y GND. |
| Botón 3 | 33 | A cablear: pulsador entre GPIO33 y GND. |
| Botón 4 | 25 | A cablear: pulsador entre GPIO25 y GND. |
| Botón 5 | 26 | A cablear: pulsador entre GPIO26 y GND. |
| Encoder 0 — CLK | 27 | |
| Encoder 0 — DT | 14 | |
| Encoder 1 — CLK | 21 | |
| Encoder 1 — DT | 22 | |

Todos los pulsadores van a **GND**, usando el pull-up interno del ESP32
(`INPUT_PULLUP`, ya configurado en el firmware) — no hace falta resistencia
externa. Los encoders usan la misma lógica de pull-up interno vía la librería
`ESP32Encoder`.

Libres para el futuro (push-button de los encoders, una 7ma tecla, etc.):
GPIO13, GPIO15 (ver nota abajo), y GPIO36-39 (sin pull-up interno — si los
usás, agregá una resistencia externa de ~10k a 3V3).

**Nota sobre GPIO15**: es un pin de "strapping" (MTDO). Usarlo como entrada
normal después del arranque no rompe nada; el único efecto es que el log de
boot por USB sale distinto. Lo marco por transparencia, no es un problema real
para este uso.

## Diagrama de conexión

```
                         TTGO T-Display (vista desde arriba)
                    ┌───────────────────────────────────┐
                    │  [pantalla 240x135]                │
       Botón 0 ─────┤ (GPIO0, integrado)                 │
       Botón 1 ─────┤ (GPIO35, integrado)                 │
                    │                                     │
   GPIO32 ──●───────┤                               ├───── USB (a la PC)
             │       │                                     │
          Botón 2    └───────────────────────────────────┘
             │
            GND

   GPIO33 ──●── Botón 3 ──GND
   GPIO25 ──●── Botón 4 ──GND
   GPIO26 ──●── Botón 5 ──GND

   Encoder 0:  CLK → GPIO27     Encoder 1:  CLK → GPIO21
               DT  → GPIO14                 DT  → GPIO22
               GND → GND (placa)            GND → GND (placa)
```

Cada pulsador: una pata a GND (cualquier pin GND de la placa), la otra al GPIO
correspondiente. Cada encoder EC11: el pin común (GND) a cualquier GND de la
placa, CLK y DT a los GPIOs indicados arriba.
