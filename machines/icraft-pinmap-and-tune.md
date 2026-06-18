# LOKLiK iCraft — pin map & motion tune

Extracted verbatim from the source machine definition
[`Grbl_Esp32/src/Device/SJMachine.h`](../Grbl_Esp32/src/Device/SJMachine.h)
(`MACHINE_NAME "LOKLIK_CRAFTER_1"`). This is the "Rosetta Stone" for rebuilding the
iCraft on FluidNC, Marlin, or grbl — the numbers below are the factory tune.

## Controller
- **MCU:** ESP32-D0WD-V3, 8 MB flash
- Stepper drivers: **TMC2209** (UART), one per axis

## Pin map (ESP32 GPIO)

| Function | GPIO |
|---|---|
| X step / dir | 19 / 18 |
| Y step / dir | 2 / 4 |
| Z step / dir | 33 / 32 |
| X limit | 25 |
| Steppers disable | 23 |
| TMC2209 UART (UART1) RX / TX | 21 / 22 |
| Pen1 / Pen2 end-stop | 39 / 34 |
| Menu key / Paper / One-key / Seek-box | 36 / 26 / 13 / 35 |
| LEDs: paper / start / pause / power | 12 / 14 / 27 / 5 |
| Power | 15 |

> Laser/spindle output pin is **not defined** in the source machine file
> (`SPINDLE_TYPE = LASER` but `SPINDLE_OUTPUT_PIN` / `LASER_OUTPUT_PIN` are
> commented out). Confirm against your hardware before wiring a laser.

## TMC2209 drivers

| Axis | UART address | Microsteps | Run current | R_sense |
|---|---|---|---|---|
| X | 0 | 16 | 0.8 A | 0.10 Ω |
| Y | 1 | 16 | 0.9 A | 0.10 Ω |
| Z | 2 | 16 | 0.9 A | 0.10 Ω |

## Motion tune

| Parameter | X | Y | Z |
|---|---|---|---|
| Steps / mm | 160 | 188.4 | 90 |
| Max rate (mm/min) | 5000 | 3000 | 5000 |
| Acceleration (mm/s²) | 2000 | 500 | 1000 |
| Max travel (mm) | 330 | (roll — effectively unlimited) | 50 |

- **Direction invert mask:** `5` (binary `101`) → **X and Z** inverted.
- **Idle lock:** steppers held (`$1 = 255`).

## Homing

| Parameter | Value |
|---|---|
| Homing enabled | yes |
| Homing cycle | X axis |
| Direction mask | 1 |
| Feed / seek rate | 500 / 3000 mm/min |
| Pull-off | 3.0 mm |
| Y homing MPos | -50 |

> On the documented unit the IR home sensor was damaged; homing was disabled
> (`$22=0`, `$21=0`) and zero set by hand. Hard/soft limits are off in the factory
> config (`DEFAULT_HARD_LIMIT_ENABLE 0`, `DEFAULT_SOFT_LIMIT_ENABLE 0`).

## Laser / spindle

- `SPINDLE_TYPE = LASER`, `DEFAULT_LASER_MODE = 1`
- PWM frequency **20 kHz** (`DEFAULT_SPINDLE_FREQ`), `DEFAULT_SPINDLE_RPM_MAX = 1000`

## Equivalent grbl `$$` settings (derived)

```
$1=255      ; idle lock (hold)
$3=5        ; dir invert mask: X + Z
$22=1       ; homing enable (set 0 if no home switch)
$23=1       ; homing dir mask
$24=500     ; homing feed (mm/min)
$25=3000    ; homing seek (mm/min)
$27=3.000   ; homing pull-off (mm)
$32=1       ; laser mode
$100=160    ; X steps/mm
$101=188.4  ; Y steps/mm
$102=90     ; Z steps/mm
$110=5000   ; X max rate (mm/min)
$111=3000   ; Y max rate
$112=5000   ; Z max rate
$120=2000   ; X accel (mm/s^2)
$121=500    ; Y accel
$122=1000   ; Z accel
$130=330    ; X max travel (mm)
$132=50     ; Z max travel (mm)
```
