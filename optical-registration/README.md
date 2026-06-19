# Optical Registration (Print-Then-Cut) — extracted & reimplemented

This folder pulls out the one genuinely clever subsystem in LOKLiK's GPL-released
firmware: the **optical registration / contour-cut alignment** that lets the
machine cut around something you already printed. Everything here is derived
from LOKLiK's own published source — nothing proprietary.

The goal: lift the *algorithm* out of their frozen Grbl_Esp32 device layer so it
can live **host-side** (in a sender / CAM app like RotatoCAM), where you actually
control it. The cut-path math doesn't need their firmware at all.

## What it does (in one paragraph)

You print your artwork on a normal printer with **three registration marks** at
the corners of a known rectangle. The machine (or a webcam, or you jogging by
eye) finds those three physical corners. From three corners you can solve a full
**affine transform** — translation, rotation, and per-axis scale — that maps
*design coordinates* onto *where the sheet actually sits on the bed*. Apply that
transform to your toolpath before cutting and the blade follows the print, even
if the paper is shifted, rotated a few degrees, or stretched 1% by the printer.

## The algorithm

Internal units in the firmware are **丝 (sī) = 0.01 mm**; converted to mm at the
motor boundary. The mark rectangle is `marker_width × marker_height`
(= 188 mm × 247 mm on the iCraft, ~A4).

1. **Find a corner precisely** (`cacleP1/P2/P3`). Each corner is an L of two
   lines. Find 2 points along the horizontal line and 2 along the vertical line,
   fit a line to each pair, and **intersect** them (`cacleIntersect`). Intersecting
   two fitted lines beats sampling the corner directly — it's robust to noise.
2. **Detect an edge while scanning** (`findMarkByRange` + `CPointQueue`). Sweep
   the sensor in one direction at 0.05 mm steps, classify each reflectance sample
   into `Paper / Plate / Marker` by threshold bins, and feed it to a small
   debouncing queue that fires only after a clean run of marker samples with
   enough paper context around it. Returns the edge location.
3. **Solve the pose** (`reviseOffset`). With corners P1 (left-top), P2 (right-top),
   P3 (right-bottom):
   - `scale_x = dist(P1,P2) / marker_width`
   - `scale_y = dist(P2,P3) / marker_height`
   - `rotation` = tilt of the P1→P2 edge off the machine X axis
   - `offset`  = P1 minus the design's mark offset
   Output is `{offset_x, offset_y, angle, scale_x, scale_y}` — a complete affine.

Note: the firmware only ever finds **3 corners** (`cacleP4()` returns empty). 3
is enough for a full affine, so the reimplementation keeps that.

## Files

```
original-source/                  verbatim extracts from LOKLiK's GPL release
  COpticalAlign.h / .cpp          the corner-find + solve logic
  CPointQueue.h / .cpp            the edge-detect debounce queue
  Common.h                        SPointInfo / SValRange / FindDir / enums
  sensor_read_SJMotorCtrl.cpp     the analogRead() hardware read (GPIO35)

reference/                        clean, hardware-free reimplementations
  optical_align.py                the affine solver — RUN THIS, it self-tests
  sensor_probe_firmware.ino       tiny ESP32 sketch: stream sensors over USB
  read_sensors.py                 PC-side live reader + marker classification
```

## Try it now (no hardware)

```
python reference/optical_align.py
```

Solves a synthetic shifted/rotated/over-scaled sheet and confirms a design point
maps back onto the real sheet to ~1e-13 mm.

## Try it with hardware wired up

1. Flash `reference/sensor_probe_firmware.ino` to an ESP32 with the optical eye
   on GPIO35 and the pen/home/paper switches wired (see the sketch header).
2. `pip install pyserial && python reference/read_sensors.py COM5`
3. Slide a printed registration mark under the eye — you'll see the class flip
   `paper → MARKER` at each edge. Re-tune the `TOLERANCE` bins to your paper/ink.

## Porting into a CAM app (RotatoCAM)

You do **not** need a raster/PNG editor or to load the printed image — the image
is just paper under the blade. The app only ever needs the **vector cut path**
(your existing SVG loader) plus **three reference points**:

- `optical_align.solve(P1, P2, P3, W, H)` → a `Registration`.
- `registration.apply(dx, dy)` on every toolpath point right before posting
  G-code → the cut lands on the print.

The three corners can come from the optical eye, a webcam, or three "jog here &
click" buttons. The solver doesn't care where they come from.

---

*All algorithm content derives from LOKLiK's GPL-released `LoklikIdeaStudio-Firmware`
(`Grbl_Esp32/src/Device/COpticalAlign.*`, `CPointQueue.*`). Read carefully, not copied blind.*
