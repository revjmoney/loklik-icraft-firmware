# Marlin Cutter Config — LOKLiK iCraft mechanics on Creality 4.2.7

**Target:** Marlin **bugfix-2.1.x / 2.1.2.x** (genuine STM32F103 4.2.7).
**Mode:** vinyl cutter / drag knife. **No endstops. Manual origin (G92).** Driven over USB
from Pronterface (or LightBurn-Marlin). Blade = Z axis, up/down via `G0 Z`.

> The drag-knife smarts (blade-offset comp, overcut, lead-in) live in the **g-code
> generator** (F-Engrave now, your own software later) — NOT in firmware. Marlin just
> runs the moves.

---

## 0. Start point
Copy the Marlin example that already targets this board:
`config/examples/Creality/Ender-3 V2/CrealityV422` **or** any Creality example whose
`MOTHERBOARD` is `BOARD_CREALITY_V427`. Copy its `Configuration.h` +
`Configuration_adv.h` into `Marlin/`, then apply the changes below.

---

## 1. Configuration.h

```cpp
// --- board / comms ---
#define MOTHERBOARD BOARD_CREALITY_V427
#define SERIAL_PORT 1            // USB (CH340)
#define BAUDRATE 115200          // bump to 250000 later if you want

// --- make it a non-printer (kills all thermal-runaway halts) ---
#define EXTRUDERS 0              // Marlin 2.1+. On older 2.0.x: set EXTRUDERS 1 and just never use E
#define TEMP_SENSOR_0 0          // no hotend
#define TEMP_SENSOR_BED 0        // no bed

// --- motion (PLACEHOLDERS — we tune in step 5) ---
#define DEFAULT_AXIS_STEPS_PER_UNIT   { 80, 80, 400, 0 }   // X belt≈80; Y feed-roller & Z UNKNOWN, measure
#define DEFAULT_MAX_FEEDRATE          { 300, 300, 40, 0 }
#define DEFAULT_MAX_ACCELERATION      { 1000, 1000, 200, 0 }
#define DEFAULT_ACCELERATION          800
#define DEFAULT_TRAVEL_ACCELERATION   800

// --- no endstops, no homing, set origin by hand ---
// Comment OUT the endstop plugs so there's no endstop logic at all:
//#define USE_XMIN_PLUG
//#define USE_YMIN_PLUG
//#define USE_ZMIN_PLUG
// Make sure this is DISABLED so you can jog without homing:
//#define NO_MOTION_BEFORE_HOMING

// --- soft limits OFF for bring-up so manual-origin jogging isn't blocked ---
//#define MIN_SOFTWARE_ENDSTOPS
//#define MAX_SOFTWARE_ENDSTOPS

// workspace (only matters once you re-enable soft limits; X is the 13"/330mm carriage)
#define X_BED_SIZE 330
#define Y_BED_SIZE 300           // Y is continuous feed — set big or leave soft limits off
#define X_MIN_POS 0
#define Y_MIN_POS 0
#define Z_MIN_POS 0

// --- niceties ---
#define EEPROM_SETTINGS          // M500 saves your tuning to flash
#define S_CURVE_ACCELERATION     // smoother corners = cleaner cuts
```

**Also turn OFF (comment out / leave undefined):** `PIDTEMP`, `FILAMENT_RUNOUT_SENSOR`,
any `*_BED_LEVELING` / `AUTO_BED_LEVELING_*`, `Z_SAFE_HOMING`, `POWER_LOSS_RECOVERY`.
With `TEMP_SENSOR_0 0` the thermal-protection sanity errors go away on their own.

---

## 2. Configuration_adv.h

```cpp
// thermal protection auto-disables with no sensor; nothing to do.

// "cool stuff" — real buttons on the host/screen via custom g-code:
#define CUSTOM_MENU_MAIN
#define CUSTOM_MENU_MAIN_TITLE "Cutter"
#define MAIN_MENU_ITEM_1_DESC "Set Origin (here)"
#define MAIN_MENU_ITEM_1_GCODE "G92 X0 Y0 Z0"
#define MAIN_MENU_ITEM_2_DESC "Blade Up"
#define MAIN_MENU_ITEM_2_GCODE "G0 Z5 F600"
#define MAIN_MENU_ITEM_3_DESC "Blade Down"
#define MAIN_MENU_ITEM_3_GCODE "G0 Z0 F300"
#define MAIN_MENU_ITEM_4_DESC "Test Square"
#define MAIN_MENU_ITEM_4_GCODE "G91\nG0 Z5\nG0 X20\nG0 Z0\nG1 Y20 F1200\nG1 X-20\nG1 Y-20\nG1 X20\nG0 Z5\nG90"
```

---

## 3. Build & flash (the SAFE path — no brick risk)
1. Build with PlatformIO/VSCode (env for the 4.2.7 / STM32F103RET6).
2. Rename the output to a **fresh** name, e.g. `firmware-cutter-001.bin` (the 4.2.7
   bootloader only flashes a filename it hasn't seen — reusing a name = it won't load).
3. Copy to a FAT32 microSD (≤32GB), insert, **power cycle**. It flashes on boot.
4. Marlin's Creality build already uses the correct **0x7000 / 28KiB** offset — that's
   why this is the no-brick path (unlike a hand-built grblHAL).

---

## 4. First power-up (Pronterface, headless)
1. Connect Pronterface to the CH340 COM @ 115200.
2. `M115` → confirm it's your Marlin build.
3. `M503` → dump settings. `M501`/`M500` as needed.
4. **Jog a tiny bit** to confirm motion & direction:
   `G91` (relative) → `G1 X5 F600` → `G1 Y5 F600` → `G1 Z2 F300` → `G90`.
   - Wrong direction on an axis → set `INVERT_X_DIR`/`Y`/`Z` (flip in firmware) or swap a coil pair.
   - Z buzzes but won't move → coil pairing wrong on the Z JST (re-pair by continuity).

## 5. Tune steps/mm (the only "calibration")
For each axis: command a known distance, measure actual, correct.
```
G91
G1 X100 F1000        ; commanded 100 mm
```
Measure real travel with a ruler. Then:
```
new_steps = old_steps × (100 / measured_mm)
M92 X<new>           ; e.g. M92 X80.6
M500                 ; save
```
- **X** (carriage belt, GT2-20T): ~80 should be close.
- **Y** (material feed roller): likely NOT 80 — grit/feed rollers have their own ratio. Measure.
- **Z** (blade up/down): unknown mechanism. Measure. Only needs ~5 mm of clean travel.

## 6. Cutting workflow
1. Load material, jog blade to the start corner.
2. **Set Origin** (custom menu) or `G92 X0 Y0 Z0`.
3. Run the F-Engrave drag-knife g-code from Pronterface.
4. Dial **kiss-cut depth** on scrap (blade-down Z value + blade pressure) before the real job.

---

## Notes
- No homing means **every session starts with manual origin** — that's fine for a cutter.
- Re-enable `MIN/MAX_SOFTWARE_ENDSTOPS` + real travel values once you trust it, to stop
  it driving into the frame.
- Keep this board on **Marlin**; gSender is out (it's grbl-only) — Pronterface/LightBurn drive Marlin.
