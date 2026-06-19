# LOKLiK iCraft — ESP32 Pin Map & Sensor Reverse-Engineering

Reverse-engineered from the GPL source LOKLiK was forced to release
(`LoklikIdeaStudio-Firmware`, a Grbl_Esp32 fork). The authoritative board
definition is **`Grbl_Esp32/src/Device/SJMachine.h`** — machine name
`LOKLIK_CRAFTER_1`. Sensor *behavior* lives in `SJMotorCtrl.cpp`,
`SJCutter.cpp`, and `COpticalAlign.cpp`.

This doc is the single source of truth for porting the iCraft to FluidNC.

---

## Full GPIO map

| GPIO | Source name | Function | Notes for FluidNC |
|------|-------------|----------|-------------------|
| **15** | `PIN_POWER` | **Soft power latch** — driven HIGH in `SJCutter` init to keep the board alive | **Critical.** Also an ESP32 strapping pin. If FluidNC doesn't assert it HIGH early, the board can power itself off. First-boot risk. |
| 19 | `X_STEP_PIN` | X step | `gpio.19` |
| 18 | `X_DIRECTION_PIN` | X dir | `gpio.18` |
| 2  | `Y_STEP_PIN` | Y step | `gpio.2` — also a boot-strapping pin, must be low at boot |
| 4  | `Y_DIRECTION_PIN` | Y dir | `gpio.4` |
| 33 | `Z_STEP_PIN` | Z step | `gpio.33` |
| 32 | `Z_DIRECTION_PIN` | Z dir | `gpio.32` |
| 23 | `STEPPERS_DISABLE_PIN` | Stepper enable (active high disable) | `gpio.23` |
| **25** | `X_LIMIT_PIN` | **X home sensor** — `LIMIT_TOUCH = 0` (active LOW) | `limit_neg_pin: gpio.25:low` — pull-up, invert. Standard homing works. |
| **39** | `PEN1_END_STOP` | Pen-1 down endstop — `PEN_TOUCH = 1` (active HIGH) | **Input-only** (GPIO34-39 have no internal pull-up). Needs external pull. Part of the Z centering routine, not a normal limit. |
| **34** | `PEN2_END_STOP` | Pen-2 down endstop — active HIGH | Input-only. See Z note below. |
| **35** | `PIN_SEEK_BOX` | **Optical registration-mark sensor** — read via `analogRead()` | Input-only, ADC1. The "blue light." Reflective Print-Then-Cut eye. No FluidNC equivalent. |
| 26 | `PIN_PAPER` | Mat/paper present switch — `PAPER_TOUCH = 0`, `INPUT_PULLUP` | Digital present/absent. NOT the optical eye. |
| 36 | `PIN_MENU_KEY` | Menu button | Input-only |
| 13 | `PIN_ONE_KEY` | One-touch start button | |
| 12 | `PIN_LED_PAPER` | Paper LED | Strapping pin (MTDI / flash voltage) — be careful driving at boot |
| 14 | `PIN_LED_START` | Start LED | |
| 27 | `PIN_LED_PAUSE` | Pause LED | |
| 5  | `PIN_LED_POWER` | Power LED | |
| 21 | `TMC_UART_RX` | TMC2209 UART RX | |
| 22 | `TMC_UART_TX` | TMC2209 UART TX | TMC ping on this UART is a first-boot risk |

### Stepper drivers
All three axes are **TMC2209** over a single UART (`UART_NUM_1`, RX=21 / TX=22),
`RSENSE = 0.1`, addresses **X=0, Y=1, Z=2**. Default currents X=0.8A, Y=0.9A, Z=0.9A,
16 microsteps.

### Motion defaults (from SJMachine.h)
- Steps/mm: **X=160, Y=188.4, Z=90**
- `DIRECTION_INVERT_MASK = 5`
- Homing: **X axis only** (`HOMING_CYCLE_1 = bit(X_AXIS)`), dir mask 1, feed 500, seek 3000, pull-off 3.0
- Travel: X max 330 mm, Z max 50 mm
- Spindle type = **LASER**, laser mode on, PWM freq 20 kHz

---

## The three things that are NOT what they look like

### 1. There is no Z limit/home sensor — and there never was
`SJMachine.h` defines exactly one limit pin: `X_LIMIT_PIN` (GPIO25). No
`Y_LIMIT_PIN`, no `Z_LIMIT_PIN`. Nothing was withheld in the GPL dump.

Z is homed by a **bespoke dual-switch centering routine**,
`SJMotorCtrl::zAxisHoming()`. It uses the two pen endstops (GPIO39 + GPIO34):
1. Rock Z right until PEN1 (39) trips,
2. Rock Z left until PEN2 (34) trips,
3. Measure how far each pen pressure-wheel travels to leave its switch,
4. Set Z=0 at the **midpoint** of the two.

It's a pressure-wheel *centering* trick, not home-to-endstop. **FluidNC can't
reproduce this from yaml.** Options for the conversion:
- Skip Z homing (bareboard yaml already does this) and set Z zero manually, **or**
- Wire one pen endstop (e.g. GPIO39) as a normal Z limit and home Z to it,
  accepting the loss of dual-pen centering. (Needs external pull-up — input-only pin.)

### 2. The blue light is an optical registration sensor, not IR / not a mat detector
`PIN_SEEK_BOX` (GPIO35), read with **`analogRead()`** — it measures reflected
light, not on/off. The blue LED illuminates the paper; a photodiode reads
reflectance. Black printed reg-marks absorb (low value), white paper reflects
(high value).

The logic is `COpticalAlign` (`COpticalAlign.cpp`). It scans X/Y for the four
corner registration marks (P1 LT, P2 RT, P3 RB, P4 LB), computes line
intersections and a skew angle (`cacleIntersect`, `cacleAngle`), then transforms
the cut path to match the printed sheet. Classic **Print-Then-Cut / contour cut**.

Hard data from the source (units 丝 = 0.01 mm):
- Sensor offset from the tool: **X +13.8 mm, Y +34.8 mm** (`sensor_offset_x=1380`, `sensor_offset_y=3480`)
- Expected reg-mark rectangle: **188 mm × 247 mm** (`marker_width=18800`, `marker_height=24700`) — ~A4

**No FluidNC equivalent.** GPIO35 works electrically (ADC1), but the scan/find/
skew logic is app-layer C++ that doesn't port. On FluidNC the blue eye goes
unused unless contour alignment is done host-side or in custom code.

### 3. Mat/paper detection is a separate, simple switch
`PIN_PAPER` (GPIO26), digital, `INPUT_PULLUP`, `PAPER_TOUCH = 0`. Just
present/absent. Don't confuse it with the optical eye (GPIO35).

---

## FluidNC port status (what transfers)

| Feature | Source | Ports to FluidNC? |
|---------|--------|-------------------|
| X homing | GPIO25, active low | ✅ Easy — `limit_neg_pin: gpio.25:low` |
| Steppers (TMC2209 UART) | UART1, addr 0/1/2 | ✅ Standard FluidNC TMC2209 setup |
| Z homing | dual pen-endstop centering | ❌ Custom routine — skip, or single-switch home |
| Optical registration | analog eye GPIO35 | ❌ App-layer, no yaml support |
| Mat present | GPIO26 digital | ⚠️ Wire as a control pin if wanted, optional |
| Power latch | GPIO15 HIGH | ⚠️ Must be asserted or board powers off |

### First-boot risk pins (watch these on the FluidNC conversion)
- **GPIO15** — power latch; board self-powers-off if not held HIGH.
- **GPIO21/22** — TMC2209 UART ping at startup.
- Strapping pins in use: 15, 2, 12 — avoid unexpected boot-time states.

---

*Derived entirely from LOKLiK's own GPL-released source. Nothing here is
proprietary — it's their published code, read carefully.*
