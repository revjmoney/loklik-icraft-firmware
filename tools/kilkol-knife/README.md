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

So you **design in LightBurn** — the UI you actually like — and still drive a drag
knife. Feed it a normal LightBurn **"Save GCode"** file and out comes FluidNC‑ready
pen code: Z up/down at the right spots, laser commands stripped, optional
**drag‑knife blade‑offset compensation + overcut** for sharp corners, optional
**multi‑pass** (two‑cut trick) for thick stock, and **material presets**.

> **Units: everything is INCHES.** Output is `G20`, so FluidNC and the CYD pendant
> stay in inches the whole job; every parameter is inches (type `0.100` for 100
> thou). LightBurn input can be any unit — it's auto‑detected (`G20`/`G21`).
> Machine Z convention: **`Z0` = UP, negative = DOWN.**

---

## Requirements
Just **Python 3** (3.8+). No pip installs — GUI uses standard‑library `tkinter`.

## Run it

**GUI** (double‑click, or `python kilkol_knife.py`): open a LightBurn `.gc`, pick a
**Material preset** (or a mode), hit **Convert**, load the result in gSender.

**CLI:**
```sh
python kilkol_knife.py input.gc output.gc --material chrome
python kilkol_knife.py input.gc output.gc --mode knife-comp --z-down -0.040 --passes 3
```

## Material presets (`--material`, or GUI dropdown)
Pick one and it fills depth / passes / feed; explicit flags still override.

| Preset | z_down | passes | feed | Notes |
|---|---|---|---|---|
| `chrome` | −0.080″ | **4** | 30 ipm | **field‑dialed** on this machine |
| `vinyl` | −0.020″ | 1 | 30 ipm | std cal sign vinyl (starting point) |
| `htv` | −0.035″ | 2 | 25 ipm | heat‑transfer (starting point) |
| `holographic` | −0.040″ | 2 | 20 ipm | slow + double cut (starting point) |

Only `chrome` is dialed in; tune the others on a scrap depth‑grid (see
**[TUNING.md](TUNING.md)**) and update the preset.

## Modes (`--mode`)

| Mode | What it does | Use for |
|---|---|---|
| `knife-comp` | pen up/down **+ blade‑offset comp + overcut** | drag knife, sharp corners, detail |
| `knife-nocomp` | pen up/down only, exact path | big/simple shapes |
| `pen-draw` | pen up/down, no comp, light touch | pen / marker drawing |

## Key options (all INCHES)

| Flag | Meaning | Default |
|---|---|---|
| `--material` | preset: fills depth/passes/feed | none |
| `--mode` | `knife-comp` / `knife-nocomp` / `pen-draw` | `knife-comp` |
| `--z-up` | retract Z (positive) — must clear the spring travel | `0.400` |
| `--z-down` | plunge / **cutting pressure** (negative) | `-0.030` (safe) |
| `--passes` | cut each shape N times (two‑cut trick) | `1` |
| `--cut-feed` | cutting speed, in/min | `30` |
| `--offset` | blade offset (comp): `0.010` @45°, `0.020` @60° | `0.010` |
| `--overcut` | overcut on closed shapes | `0.040` |
| `--cutoff` | corner angle (deg) that triggers a swivel arc | `20` |
| `--g1-travel` / `--no-home` | G1 travels / skip return to `X0 Y0` | off |

## LightBurn side
- Cut layers in **Line mode** (NOT Fill/scan — Fill rasterizes, useless for a knife).
- **File → Save GCode** (any GRBL device profile).
- **Units & output mode don't matter** — auto‑detects inch (`G20`)/mm (`G21`) and
  absolute (`G90`)/relative (`G91`), and **always writes inches (G20)**.
  Power/speed are ignored (it uses your feed/depth).

### Cut Optimization (LightBurn → **Optimization Settings**)
Tuned for drag-knife **sticker / kiss-cuts**:
- **Cut inner shapes first — ON.** *The important one.* Interior detail/holes get
  cut before the outline, so the piece can't shift or lift before the inside is done.
- **Reduce travel moves — ON.** Less wasted rapid travel.
- **Choose best direction — ON.** Fine — the blade-offset comp handles either way.
- **Remove overlapping lines — ON** (~0.025 mm). Stops the knife re-cutting shared edges.
- **Choose best starting point — ON** (optional). Starts each loop on a straight,
  not a corner, so the overcut seam lands cleanly.
- **Cut in direction order — OFF.** Rigid spatial sweep fights inner-first.
- **Hide backlash — OFF.** Laser-engraving feature; useless for a knife.
- **Reduce direction changes — OFF.** Engraving-oriented; inner-first is what you want.
- **Order by Layer** is all a sticker sheet needs. Then hit **Set as Defaults**.

## Z zero & touch‑off (the rolling‑paper trick)
No Z home switch, so you set Z zero by hand each session — fast and repeatable:

1. Load material.
2. Lay a **single rolling paper on top** as a feeler (a **Job 1.5** ≈ **0.002″**,
   dead consistent — and the best cheap feeler gauge in the shop 😉).
3. Jog Z **down** in small (~0.005″) steps until the knife just **pinches the
   paper** — first drag, not a hard press.
4. Hit **Zero**.

That puts `Z0` a hair above the surface. The output **lifts to `z_up` (+0.400″)
before any X/Y move and after every cut**, so the blade never drags across the
material — `z_up` must be **bigger than the holder's spring travel** to fully
extract the blade (this machine's spring is ~0.2″, hence 0.400″).

> Watch **WPos** on the pendant, not MPos. WPos reads 0 at your touch‑off; MPos is
> the machine‑absolute count and only means anything after homing (which this
> machine doesn't do). MPos ≠ WPos is normal — that gap *is* the work offset.

## Dialing the cut (short version)
On a **soft spring holder, `z_down` is cutting PRESSURE**, not depth (the blade
exposure caps depth). Thick/chrome wants **shallow depth + 2–3 passes**, not one
deep plunge. To dial a material: cut a **scrap depth‑grid** (small squares,
stepping depth and/or passes) and pick the shallowest that severs the film clean
and leaves the backing — then bake it into a `--material` preset.

**Read the cut:** a correct cut leaves only a **faint matte scratch on the backing
liner** — never through it.

**Full guide — spring force calibration, blade angles (45° vs 60°/chrome),
material force table, multi‑pass rationale: [TUNING.md](TUNING.md).**

## How the comp works
LightBurn emits flattened line segments, so the comp reconstructs each cut into a
polyline and (1) shifts every commanded (holder) point **forward along travel by
the offset** so the *trailing* blade tip lands on the line, and (2) inserts a
small **swivel arc** (radius = offset) at sharp corners so the blade re‑aims
*through* the corner instead of rounding it. Ported from
**[Inkcut](https://github.com/codelv/inkcut)** (GPLv3).

## License
**GPLv3** — see the repository `LICENSE`. Blade‑offset comp derived from Inkcut
(GPLv3, © Jairus Martin).
