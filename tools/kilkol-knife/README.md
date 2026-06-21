# KilKol Knife

**LightBurn G‑code → drag‑knife / pen G‑code for FluidNC.**
*Part of the Simian Tactical Toolbox — Rev. J. Money, [jmscnc.com](https://jmscnc.com)*

LightBurn is a laser program, but it already does the one thing a vinyl cutter
needs: it splits every job into **travel** (laser off) and **cut** (laser on).
That maps exactly onto a drag knife or pen:

| LightBurn | Knife / pen |
|---|---|
| laser **on** (`G1`, power `S>0`) | blade/pen **DOWN** |
| laser **off** (`G0`, or `S0`/`M5`) | blade/pen **UP** |

So you get to **design in LightBurn** — the UI you actually like — and still
drive a drag knife. Feed this tool a normal LightBurn **"Save GCode"** file and
it spits out FluidNC‑ready pen code: Z up/down inserted at the right spots, the
laser commands stripped, and (optionally) real **drag‑knife blade‑offset
compensation + overcut** so your corners come out sharp instead of rounded.

> Machine Z convention (LokLik iCraft on FluidNC): **`Z0` = UP, negative = DOWN.**

---

## Requirements
Just **Python 3** (3.8+). No pip installs — the GUI uses the standard‑library
`tkinter`, the rest is pure Python.

## Run it

**GUI** (double‑click, or):
```sh
python kilkol_knife.py
```
Open a LightBurn `.gc`, pick a mode, hit **Convert**, load the result in gSender.

**CLI:**
```sh
python kilkol_knife.py input.gc output.gc --mode knife-comp --offset 0.25 --overcut 1.0
```

## Modes

| Mode | What it does | Use for |
|---|---|---|
| `knife-comp` | pen up/down **+ blade‑offset compensation + overcut** | drag knife, sharp corners, small/intricate detail |
| `knife-nocomp` | pen up/down only, exact path | drag knife, big/simple shapes |
| `pen-draw` | pen up/down, no comp, light touch, faster feed | pen / marker drawing (a pen has no blade offset) |

## Key options

| Flag | Meaning | Default (per mode) |
|---|---|---|
| `--mode` | `knife-comp` / `knife-nocomp` / `pen-draw` | `knife-comp` |
| `--z-up` | pen‑up Z (your "up" is `0`) | `0` |
| `--z-down` | plunge depth, mm **negative** | `-1.5` knife / `-1.0` pen |
| `--cut-feed` | cutting speed, mm/min | `800` knife / `1500` pen |
| `--offset` | blade offset, mm (comp mode) | `0.25` |
| `--overcut` | overcut on closed shapes, mm | `1.0` |
| `--cutoff` | corner angle (deg) that triggers a swivel arc | `20` |
| `--g1-travel` | use `G1` travels instead of `G0` rapids | off |
| `--no-home` | don't return to `X0 Y0` at the end | off |

## LightBurn side
- Set your cut layers to **Line mode** (NOT Fill/scan — Fill rasterizes, useless
  for a knife).
- **File → Save GCode** (any GRBL device profile).
- **Units & output mode don't matter** — the tool auto-detects inch (`G20`) vs
  mm (`G21`) and absolute (`G90`) vs "current position" relative (`G91`), and
  always writes mm. Power/speed are ignored (it uses your `--cut-feed` /
  `--z-down`).

## Tuning the blade (comp mode)
- **Blade offset ≈ the blade spec:** 45° blade ≈ **0.25 mm**, 60° ≈ **0.5 mm**.
- Cut a test square, read the corners:
  - **Rounded** → offset too **low** → +0.05 mm.
  - **Little horns/flares** at corners → offset too **high** → −0.05 mm.
- **Overcut** ~0.25–1 mm; raise it if closed shapes don't fully separate at the
  start/stop point.
- **Plunge (`z-down`):** start shallow; deepen until it cuts the vinyl but not
  the backing.

## How the comp works
LightBurn outputs flattened line segments, so the compensation reconstructs each
cut into a polyline and:
1. shifts every commanded (holder) point **forward along travel by the offset**,
   so the *trailing* blade tip lands on the design line, and
2. inserts a small **swivel arc** (radius = offset) at corners sharper than the
   cutoff, so the blade re‑aims *through* the corner instead of rounding it.

The blade‑offset algorithm is ported from **[Inkcut](https://github.com/codelv/inkcut)** (GPLv3).

## License
**GPLv3** — see the repository `LICENSE`. Blade‑offset comp derived from Inkcut
(GPLv3, © Jairus Martin).
