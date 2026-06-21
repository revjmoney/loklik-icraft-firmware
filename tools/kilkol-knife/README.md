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

## Z zero & touch-off (the rolling-paper trick)

This machine has no Z home switch, so you set Z zero **by hand each session** —
quick and repeatable:

1. Load your vinyl.
2. Lay a **single rolling paper on top of the vinyl** as a feeler gauge. (A
   **Job 1.5** is perfect — ~0.025 mm and dead consistent. And yes, I keep
   rolling papers in the shop because I'm a grown adult and allowed to 😉 — they
   also happen to be the best cheap feeler gauge money can buy.)
3. Jog Z **down** in small (~0.1 mm) steps until the knife just **pinches the
   paper** — *first drag, not a hard press.* Your Z is a rigid leadscrew, so
   jamming it into the material deflects the gantry / skips steps and gives a
   false-deep zero.
4. Hit **Zero** on the pendant.

That puts `Z0` a hair **above** the film surface, which is why the defaults are:
- **`z_up` POSITIVE** (`+2`) — lift to clear before traveling.
- **`z_down` NEGATIVE** (`-0.85`) — the plunge below `Z0`.

**Spring‑loaded holder?** Then `z_down` mostly sets **cutting pressure**, not
depth — your **blade exposure** (the holder cap) caps how deep the blade actually
goes, and the spring keeps pressure steady over an uneven sheet. So a deeper
`z_down` = *more punch*. (Enough extra still nudges a spring blade toward the
backing, so there's a sweet spot.) The `-0.85` default came from a depth‑ladder
test and sits just above the **factory iCraft's ~0.8 mm plunge** — nice
independent confirmation. **It's fully adjustable per job** — change the
**"Z down / pressure"** field (GUI) or `--z-down` (CLI); no code editing.

**Safety:** the output always lifts Z to `z_up` **before any X/Y move** (and after
every cut), so the knife never drags across freshly loaded material — as long as
`z_up` stays positive.

> Watch **WPos** on the pendant, not MPos. WPos reads 0 at your touch-off; MPos is
> the machine-absolute count and only means anything after homing (which this
> machine doesn't do). MPos differing from WPos is normal — that gap *is* the work
> offset.

## Tuning the blade (comp mode)
- **Blade offset ≈ the blade spec:** 45° blade ≈ **0.25 mm**, 60° ≈ **0.5 mm**.
- Cut a test square, read the corners:
  - **Rounded** → offset too **low** → +0.05 mm.
  - **Little horns/flares** at corners → offset too **high** → −0.05 mm.
- **Overcut** ~0.25–1 mm; raise it if closed shapes don't fully separate at the
  start/stop point.
- **Plunge / pressure (`z-down`):** default `-0.85` (rolling-paper touch-off +
  spring holder). Tune on scrap — **shallower** if it scores the backing,
  **deeper** if it doesn't weed clean.
- **Dial a new material fast with a depth ladder:** cut a row of small squares,
  each a step deeper (e.g. `-0.3, -0.4, … -0.9`), then weed them — the
  **shallowest one that lifts clean (backing intact)** is your number. Beats
  re-cutting the whole job to guess.

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
