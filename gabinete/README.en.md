**English** | [Español](README.md)

# Printable case

![Assembly](img/ensamble.png)

Parametric design in OpenSCAD (`gabinete.scad`). Outside size:
**94.8 × 93.8 × 20.4 mm** (without the leg or the knobs/keycaps). File and
part names are in Spanish: *tapa* = lid, *base* = base, *bisagra* = hinge,
*pata* = leg, *pulsador* = plunger, *prueba* = test piece.

- 6 Cherry MX in a 3 × 2 grid at the front, snapped into the lid (15 × 15 mm
  cutout, tuned with a test print, with a 1.5 mm "plate"). The pins stick out
  underneath so you can solder the wires straight to them, no PCB. Numbering
  (same as the app and the screen): back row 1-2-3, front row 4-5-6.
- 2 EC11 at the back left (9.4 mm hole, loose fit); the lid is 1.6 mm thick
  there so the washer + nut fit on the M7 bushing.
- The TTGO snaps in from below into a frame with clips, with the display
  against the window. The USB-C comes out on the right side. The frame's long
  walls have a 6 mm opening 10.5 mm from the USB end for the board's side
  button, and the underside of the lid has a recess between the plungers for
  the USB-C connector body.
- 2 loose plungers go through the lid and press the board's built-in buttons
  (GPIO0 and GPIO35): they're buttons 7 and 8, also assignable from the app
  (previous/next layer by default).
- Rear leg with a toothed hinge: loosen the screw, pick the angle (one
  "click" every 15°) and tighten it. Folded it gives ~4° of tilt; upright,
  ~29°.

## Before printing everything: measure and test

The TTGO measurements were tuned with test prints on the author's board
(T-Display V1.1). Boards can vary, so it's worth checking with calipers and
adjusting at the top of `gabinete.scad`:

| Parameter | What to measure |
|---|---|
| `pcb_l`, `pcb_w` | PCB length and width |
| `stack_top` | from the top face of the PCB to the display glass |
| `btn_h` | from the top face of the PCB to the top of the buttons |
| `btn_dx`, `btn_dy` | button position relative to the PCB center |
| `disp_dx`, `disp_w`, `disp_h` | position and size of the visible area |

Then print the two test pieces (~15 min each):

- `prueba_ttgo.stl`: the section of the lid with the TTGO frame, the window
  and the plungers. Checks that the board clips in, the display is centered
  and the plungers press the buttons.
- `prueba_mx_enc.stl`: one switch and one encoder. Checks that the Cherry
  clips in firmly: if it's too tight, raise `mx_cut`; if it's loose, lower it
  in 0.2 mm steps (the standard plate cutout is 14.0).

The display window uses the measurements of a proven TTGO case:
[TTGO case by kamma_cz](https://www.thingiverse.com/thing:4983880) (CC BY):
28.8 × 17 mm, starting 7.1 mm from the PCB edge opposite the USB.

Adjustments already made after the test prints: `mx_cut` = 15.0,
`frame_extra_w` = 1.0 (frame 1 mm wider), the side-button opening
(`side_btn_gap`, `side_btn_from_end`), the USB-C connector recess
(`usb_conn_l`, `usb_conn_w`) and `enc_hole` = 9.4.

## Parts (`stl/`, already oriented for printing)

| File | Qty | Notes |
|---|---|---|
| `tapa.stl` (lid) | 1 | Printed upside down (the visible face goes on the bed). |
| `base.stl` | 1 | No supports. |
| `bisagra.stl` (hinge) | 1 | Knuckles up, no supports. |
| `pata.stl` (leg) | 1 | Lying flat, no supports. |
| `pulsador.stl` (plungers) | 1 (contains 2) | Standing; if the flange comes out ugly, use a brim. |

PLA or PETG, 0.2 mm layers, 3 perimeters, 20 % infill.

## Hardware

- 4 × M3×12 (lid → base, screwed in from below into the lid's bosses;
  self-tapping or metric, the hole is 2.6 mm).
- 2 × M3×8 (hinge → base).
- 1 × M3×25 + 1 M3 nut (hinge axle; the nut sits in the hex pocket).
- 2 adhesive rubber feet up to 10 mm in diameter (front).

## Assembly

1. Put the 2 plungers into their holes from under the lid (the flange sits
   inside the recess).
2. Snap the TTGO into the frame from below, display toward the window and
   USB-C toward the right side, until the clips click.
3. Mount the 6 Cherry MX from above and the 2 EC11 from below (washer + nut
   on top). If your EC11 have the small anti-rotation tab, bend it or cut it
   off: the lid has no slot for it.
4. Solder the wires following [`docs/wiring.en.md`](../docs/wiring.en.md).
5. Close it with the base and the 4 M3×12.
6. Screw the hinge under the base, put the leg between the knuckles and pass
   the M3×25 through with the nut.

## Regenerating the STLs

```bash
openscad -D 'pieza="tapa"' -o stl/tapa.stl gabinete.scad
# parts: tapa, base, bisagra, pata, pulsador, prueba_ttgo, prueba_mx_enc
# "ensamble" (default) shows everything assembled; angulo_pata rotates the leg
```
