**English** | [Español](wiring.md)

# Wiring — TTGO T-Display (ESP32)

Board confirmed by probing it directly (`esptool chip-id`): **ESP32-D0WDQ6-V3**,
no native USB (CH9102 serial bridge), 1.14" ST7789 240x135 display.

## Parts list

- 6x Cherry MX switches (Brown), plate mount. They snap into the
  [case](../gabinete/README.en.md) lid, with the wires soldered straight to
  the pins.
- 2x EC11 rotary encoders, **preferably with a push switch** (5 pins: the 3
  rotation pins + the 2 pins of the shaft switch). Without the switch (3
  pins) works too: see the note below.
- Thin wire for soldering (wire-wrap or ribbon cable).
- The board's 2 built-in buttons are used as buttons 7 and 8 (through the
  case's printed plungers); nothing to buy for those.

## Pins used by the board (don't touch)

| Function | GPIO |
|---|---|
| Display MOSI | 19 |
| Display SCK | 18 |
| Display CS | 5 |
| Display DC | 16 |
| Display RST | 23 |
| Display BL (backlight) | 4 |
| Battery (ADC) | 34 |
| Battery ADC enable | 14 (not on the header) |
| USB serial (UART0 TX/RX) | 1 / 3 |

## Macropad pinout

Button numbers match the app (Key 1-6, Board, Encoder press) and the layer
screen. In the case, the back row is 1-2-3 and the front row 4-5-6, left to
right.

| Function | GPIO | Notes |
|---|---|---|
| Key 1 (back, left) | 32 | Cherry MX between GPIO32 and GND. |
| Key 2 (back, center) | 33 | Cherry MX between GPIO33 and GND. |
| Key 3 (back, right) | 25 | Cherry MX between GPIO25 and GND. |
| Key 4 (front, left) | 26 | Cherry MX between GPIO26 and GND. |
| Key 5 (front, center) | 13 | Cherry MX between GPIO13 and GND. |
| Key 6 (front, right) | 17 | Cherry MX between GPIO17 and GND. |
| Board (GPIO0) | 0 | Built in. Careful: if you hold it while plugging in USB or resetting, the board enters flashing mode instead of booting normally. In normal use it works like any other button. |
| Board (GPIO35) | 35 | Built in. |
| Encoder 1 (back) — CLK | 27 | |
| Encoder 1 (back) — DT | 12 | Strapping pin: see the note below. |
| Encoder 2 (front) — CLK | 21 | |
| Encoder 2 (front) — DT | 22 | |
| Encoder 1 (press) | 15 | Encoder 1 shaft switch: one pin to GPIO15, the other to GND. |
| Encoder 2 (press) | 2 | Encoder 2 shaft switch: one pin to GPIO2, the other to GND. |

> **Does your encoder have no push switch?** The EC11 is sold with a push
> switch (5 pins: 3 on one side, 2 on the other) and without one (3 pins
> only); cheap clones come both ways too. Without the switch everything else
> works the same: turning works normally, and buttons 9 and 10
> ("Encoder 1/2 (press)") simply don't exist. Leave them on "None" and don't
> wire GPIO15 or GPIO2. Left unconnected, those pins stay idle (internal
> pull-up), so they won't produce phantom presses.

The EC11 has 3 pins on one side (A – C – B: rotation, C is the common pin to
GND) and 2 on the other (the push switch, no polarity). The 2 large metal
legs are only for mounting.

Every switch goes to **GND**, using the ESP32's internal pull-up
(`INPUT_PULLUP`, already set up in the firmware). No external resistors or
diodes are needed (each key has its own GPIO, there's no matrix). The
encoders use the same internal pull-up through the `ESP32Encoder` library.

Free for future use: GPIO36-39 (no internal pull-up: if you use them, add an
external ~10k resistor to 3V3). Note that GPIO14 is **not** on the
T-Display header: the board uses it internally for the battery reading.

**About GPIO12, GPIO15 and GPIO2**: these are strapping pins (the ESP32 reads
them at boot). The encoder and the switches only pull them to GND, never to
3V3, which doesn't change a normal boot (on GPIO12 the dangerous case would
be pulling it to 3V3, which is why it goes to the encoder, which only
connects to GND). Just don't hold encoder 1 pressed while resetting (the boot
log comes out different, nothing more), or encoder 2 while flashing over USB.

## Connection diagram

```
   Top view of the case

   (Enc 1)                 ┌──────────── TTGO ────────────┐ [7]
                           │       [screen 240x135]        ├─ USB
   (Enc 2)                 └──────────────────────────────┘ [8]

        [1]  [2]  [3]          [7] = GPIO0, [8] = GPIO35 (built in,
        [4]  [5]  [6]                 through the printed plungers)

   Key 1 ── GPIO32          Key 4 ── GPIO26
   Key 2 ── GPIO33          Key 5 ── GPIO13
   Key 3 ── GPIO25          Key 6 ── GPIO17
   (the other leg of each switch goes to GND)

   Encoder 1:  CLK → GPIO27     Encoder 2:  CLK → GPIO21
               DT  → GPIO12                 DT  → GPIO22
               C   → GND                    C   → GND
               SW  → GPIO15                 SW  → GPIO2
               SW  → GND                    SW  → GND
```

Tip: make a GND "bus" by chaining one wire through the GND leg of the 6
switches, the common pin of the 2 encoders and one pin of each encoder's push
switch, and run that bus to a single GND pin on the board.
