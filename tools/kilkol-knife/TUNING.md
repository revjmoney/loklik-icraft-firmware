# Tuning the cut — blade depth, pressure, blades & passes

Practical guide for dialing cuts on the iCraft (FluidNC) with a **soft spring‑loaded
drag‑knife holder** on a leadscrew Z. Everything is in **inches / thou**.

---

## The one thing to understand: on a spring holder, Z depth **is** force

Unlike a stock plotter (separate "depth" and "pressure" knobs), your holder turns
Z over‑travel into downforce through the spring. It's just the spring law:

> **F = F_preload + k · x**  — where `x` = how far Z plunges **past** the point the
> blade tip touches the surface, `k` = spring rate, `F_preload` = force when it
> just kisses.

So **`z_down` in KilKol Knife = cutting pressure.** Deeper plunge ≠ deeper gouge
(the blade exposure caps depth, see below) — it just presses harder.

A measured example from a comparable CNC drag‑knife build: **≈ 375 g + 57 g/mm**
(≈ **1.4 g per 0.001″**). Yours will differ — measure it (next section).

## Calibrate your spring once (turns Z into real grams)

This is the single highest‑value thing you can do — it converts every "pressure"
number in the trade into a commanded Z for *your* machine:

1. Rest the blade tip on a **gram scale** (kitchen/postal).
2. Zero the scale, then **jog Z down in known steps** (e.g. 0.005″ / 0.010″).
3. Record grams at each step → you now have your own `F = F0 + k·x` line.

Pick a target force from the tables below, read off the matching Z, done.

## Blade exposure — set ONCE, ≈ material thickness

- Expose only **~the material thickness** (just enough to sever the film, not the
  liner). Rule of thumb: **"feel it, don't see it"** — about half a credit‑card
  thickness proud (~0.003″).
- Exposure is a **hard mechanical stop**: once the holder nose bottoms on the
  surface, the blade can't go deeper no matter the force. That's *why* a spring
  holder is forgiving — extra Z can't reach the liner, it only adds pressure.
- **Never** dial cut depth by re‑extending the blade — set exposure to thickness,
  then trim force with `z_down`. Over‑extending the blade is the #1 mistake.

## Blade angle — 45° vs 60° (chrome wants 60°)

| Angle | Best for | Notes |
|---|---|---|
| **30°** | Fine detail / small text on **thin** stock (thin cal, std HTV) | Weak tip, won't do thick |
| **45°** | **All‑purpose** — cal vinyl, std HTV, paper up to ~4 mil | Struggles on glitter/flock/chrome |
| **60°** | **Thick/dense — chrome, metallic, reflective, glitter, flock** | Worse on tiny intricate detail |

> **If chrome is fighting you on a 45°, switch to a 60°.** More cutting edge is
> engaged; it carves thick film a 45° just mashes — often turning a 3‑pass fight
> into one clean pass.

**Blade offset** (set in KilKol Knife `--offset`) is per‑angle: **~0.010″ (0.25 mm)
for 45°, ~0.020″ (0.5 mm) for 60°.** Too low = rounded corners; too high = notched.

## Two‑cut trick (multi‑pass) — when & why

Use `--passes 2` (or 3) for **thick/dense** material (chrome, metallic, glitter,
flock, heavy cardstock) or any time one pass leaves an incomplete/hard‑to‑weed cut.

Two shallow passes beat one deep plunge because:
1. **Less blade deflection / side‑load** → sharper corners, blade stays on path.
2. **Progressive engagement** → cleaner sever, longer blade + mat life.
3. **No over‑forcing** → a single deep pass needs huge force that mashes edges
   into the liner and ruins weeding.

On a spring holder a single pass on thick chrome may simply not reach the liner
at any force (nose bottoms out) — a 2nd pass is the only way through. That's what
the `passes` setting is for.

## Material starting table

Translate the force column to **your** commanded Z using your gram calibration.
Force is **blade‑condition dependent** — a fresh blade needs **~20–30 % less**
(drop it when you swap blades or you'll cut the liner).

| Material | Angle | Exposure | Force (sharp blade) | Passes |
|---|---|---|---|---|
| Premium cast cal (2 mil) | 45° | ~0.002–0.003″ | low (~140 g) | 1 |
| Std cal / Oracal 651 (3 mil) | 45° | ~0.003″ | ~60–210 g | 1 |
| Std HTV (EasyWeed) | 45°/30° | ~0.0035″ | low (~40–60 g) — *don't over‑force* | 1 |
| Reflective / metallic / **chrome** | **60°** | ~0.005–0.008″ | high | **2** |
| Glitter / flock HTV | **60°** | thickness | high | **2** |
| Sandblast / stencil mask | **60°** | thickness | high | 1–2 |

KilKol Knife `--material` presets (`chrome` is field‑dialed; rest are starting
points — tune on a scrap depth‑grid).

## Dialing a new material (depth‑grid method)

Don't guess on the real job — cut a **scrap grid**: a row/grid of small squares,
each a step deeper (and/or more passes). Weed each. **The shallowest square that
severs the film clean and leaves the backing intact = your number.** Then bake it
into a `--material` preset.

## Reading the cut

A correct cut leaves only a **faint matte scratch / line on the backing liner** —
**never** through it. Cuts the liner = too much force (less Z) or too much
exposure. Won't weed clean = not enough (more Z, or add a pass, or fresh/sharper
blade, or step up to 60°).

## How it maps to KilKol Knife

| Setting | What it is |
|---|---|
| `z_down` | **cutting pressure** (spring over‑travel), negative inches |
| `z_up` | retract — must clear the spring's travel (~0.4″ here) so the blade lifts |
| `passes` | the two‑cut trick (2–3 for thick/chrome) |
| `offset` | blade‑offset comp — match the blade angle (0.010″ @45°, 0.020″ @60°) |
| `--material` | preset that fills depth/passes/feed |

---

## Sources
- Spring drag‑knife force mechanism: <https://mcuoneclipse.com/2021/06/27/diy-vinyl-cutting-drag-knife-for-desktop-cnc/>
- Downforce calibration, real spring numbers (375 g + 57 g/mm): <https://softsolder.com/2020/03/23/drag-knife-calibration-downforce-and-speed/>
- Blade depth vs pressure, gram scale: <https://signoftimess.com/2015/05/16/blade-depth-and-pressure/>
- Blade angles / offset / exposure: <https://www.thewhblog.com/blog/how-to/blade-angles-shapes/>
- 45° vs 60° vs 30° by material: <https://atlantavinylstore.com/blogs/news/what-is-the-difference-between-45-and-60-degree-blades-for-vinyl-cutters>
- Cutter settings, force vs depth (Roland): <https://www.rolanddga.com/blog/what-should-my-cutter-settings-be>
- Graphtec gram‑force / blade condition / offset: <https://www.t-shirtforums.com/threads/24-graphtec-blade-depth-and-pressure.96163/>
- Metallic/chrome HTV settings: <https://www.siserna.com/files/htv-cutter-settings.pdf>
- Multi‑pass rationale: <https://support.carvewright.com/the-drag-knife/> · <https://www.sollex.com/blog/post/drag-knife-for-cnc-digital-cutters-and-plotters-guide>
