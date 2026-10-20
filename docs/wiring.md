# Cableado — TTGO T-Display (ESP32)

Placa confirmada por sondeo directo (`esptool chip-id`): **ESP32-D0WDQ6-V3**, sin USB
nativo (puente serie CH9102), display ST7789 1.14" 240x135.

## Lista de compras

- 6x switch Cherry MX (Brown), montaje en placa — van encastrados en la tapa del
  [gabinete](../gabinete/README.md), con los cables soldados directo a los pines.
- 2x encoder rotativo EC11 (solo se usan las 2 señales de cuadratura CLK/DT; el
  push-button del eje, si el modelo lo trae, queda sin cablear por ahora).
- Cable fino para soldar (tipo wire-wrap o de cinta plana).
- Los 2 botones integrados de la placa se usan como botones 7 y 8 (vía los
  pulsadores impresos del gabinete), no hace falta comprar nada para eso.

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

La numeración de botones es la de la app (Tecla 1-6, Placa) y la pantalla de
capa. En el gabinete, la fila de atrás es 1-2-3 y la de adelante 4-5-6, de
izquierda a derecha.

| Función | GPIO | Notas |
|---|---|---|
| Tecla 1 (atrás, izq.) | 32 | Cherry MX entre GPIO32 y GND. |
| Tecla 2 (atrás, centro) | 33 | Cherry MX entre GPIO33 y GND. |
| Tecla 3 (atrás, der.) | 25 | Cherry MX entre GPIO25 y GND. |
| Tecla 4 (adelante, izq.) | 26 | Cherry MX entre GPIO26 y GND. |
| Tecla 5 (adelante, centro) | 13 | Cherry MX entre GPIO13 y GND. |
| Tecla 6 (adelante, der.) | 17 | Cherry MX entre GPIO17 y GND. |
| Placa (GPIO0) | 0 | Integrado. Ojo: si lo mantenés apretado al conectar el USB o resetear, la placa entra en modo de flasheo en vez de arrancar normal. En uso normal funciona como botón cualquiera. |
| Placa (GPIO35) | 35 | Integrado. |
| Encoder 1 (atrás) — CLK | 27 | |
| Encoder 1 (atrás) — DT | 14 | |
| Encoder 2 (adelante) — CLK | 21 | |
| Encoder 2 (adelante) — DT | 22 | |

Todos los switches van a **GND**, usando el pull-up interno del ESP32
(`INPUT_PULLUP`, ya configurado en el firmware) — no hace falta resistencia
externa ni diodos (cada tecla tiene su propio GPIO, no hay matriz). Los
encoders usan la misma lógica de pull-up interno vía la librería
`ESP32Encoder`.

Libres para el futuro (push-button de los encoders, etc.): GPIO15 (ver nota
abajo), GPIO2/GPIO12 (también de "strapping", mejor evitarlos) y GPIO36-39
(sin pull-up interno — si los usás, agregá una resistencia externa de ~10k a 3V3).

**Nota sobre GPIO15**: es un pin de "strapping" (MTDO). Usarlo como entrada
normal después del arranque no rompe nada; el único efecto es que el log de
boot por USB sale distinto.

## Diagrama de conexión

```
   Vista superior del gabinete

   (Enc 1)                 ┌──────────── TTGO ────────────┐ [7]
                           │       [pantalla 240x135]      ├─ USB
   (Enc 2)                 └──────────────────────────────┘ [8]

        [1]  [2]  [3]          [7] = GPIO0, [8] = GPIO35 (integrados,
        [4]  [5]  [6]                 vía pulsadores impresos)

   Tecla 1 ── GPIO32        Tecla 4 ── GPIO26
   Tecla 2 ── GPIO33        Tecla 5 ── GPIO13
   Tecla 3 ── GPIO25        Tecla 6 ── GPIO17
   (la otra pata de cada switch, a GND)

   Encoder 1:  CLK → GPIO27     Encoder 2:  CLK → GPIO21
               DT  → GPIO14                 DT  → GPIO22
               C   → GND                    C   → GND
```

Tip: hacé una "barra" de GND encadenando con un solo cable la pata de GND de
los 6 switches y el pin común de los 2 encoders, y llevá esa barra a un solo
pin GND de la placa.
