# Machine definitions

Everything you need to rebuild, repair, or convert a LOKLiK iCraft — derived from
the factory firmware source.

---

## 🚨 Call for Community Help & Contributions

The core motion system works — **this project now needs your help to map the rest of the hardware.**

**What we already know** (reverse-engineered from the factory `Grbl_Esp32` source, `src/Device/SJMachine.h`):

| Function | Pin | Status |
|---|---|---|
| X limit / home | `GPIO25` | Known — but the IR home sensor is commonly **dead** (surge / teardown); verify or replace before enabling X homing |
| Z limit | `GPIO39` | Known (input-only pin, fine as a limit) |
| Y limit | — | Factory has none (Y is continuous material feed) |
| Cycle-start button | `GPIO13` | Known (internal pull-up) |
| Feed-hold button | `GPIO36` | Known |
| Seek / box button | `GPIO35` | Known (floats, reads active at boot) |

These are mapped **but disabled** in the configs here (`must_home: false`, `hard_limits: false`) for safe bring-up — not because they're unknown.

**What we still need — this is where you come in:**
- **Mat / material-presence sensor** and **tool-depth sensor** — not in the factory pin list; trace them with a multimeter / logic analyzer.
- **Confirm or repair the X IR home sensor** so X homing can be re-enabled.
- Any other limit / sensor inputs on your specific iCraft revision.

**How to help:** trace a sensor back to its GPIO → add a sensor-enabled YAML to this `machines/` folder via PR → or open an issue with what you find.

---

| File | What it is |
|---|---|
| [`icraft-pinmap-and-tune.md`](icraft-pinmap-and-tune.md) | The factory pin map, TMC2209 settings, and motion tune (steps/mm, rates, accel, homing) pulled from `src/Device/SJMachine.h`. Includes the equivalent grbl `$$` settings. |
| [`icraft-bareboard-nohoming.yaml`](icraft-bareboard-nohoming.yaml) | **FluidNC config tested on a real iCraft board (v4.0.3).** Factory motion tune, RMT stepping, no homing / no sensors — motors + front-panel buttons only. Start here. See [`../FLASH_FLUIDNC.md`](../FLASH_FLUIDNC.md). |
| [`icraft-fluidnc-config.yaml`](icraft-fluidnc-config.yaml) | Earlier **FluidNC** config reproducing the factory tune (reference). Note: uses `I2S_static` and `r_sense 0.110`; the bare-board file above corrects these to `RMT` / `0.100` after on-hardware testing. |
| [`marlin-cutter-config.md`](marlin-cutter-config.md) | Notes for running the machine on **Marlin** (e.g. a Creality 4.2.7 / STM32 board). |
| [`diy-esp32-tb6600-fluidnc-config.yaml`](diy-esp32-tb6600-fluidnc-config.yaml) | FluidNC config for a **bare ESP32 + TB6600** external-driver build (3-axis). |
| [`diy-esp32-tb6600-wiring.md`](diy-esp32-tb6600-wiring.md) | Common-anode ESP32→TB6600 wiring, pin map, and bring-up steps. |

## Which firmware should I use?

| Route | Board | Firmware | Notes |
|---|---|---|---|
| Stay stock-but-unlocked | the iCraft's own ESP32 | this repo's Grbl_ESP32 | flash a clean image, talk G-code |
| Upgrade | the iCraft's own ESP32 | **FluidNC** | YAML config, no recompile; flash [`../fluidnc-firmware/`](../fluidnc-firmware/) + use `icraft-bareboard-nohoming.yaml` ([guide](../FLASH_FLUIDNC.md)) |
| Reuse a printer board | Creality 4.2.7 (**STM32**) | **Marlin** | FluidNC will NOT run on STM32 |
| PC-based | breakout board | Mach3/grbl | older; overkill for a cutter |

> The factory machine names are `LOKLIK_CRAFTER_1` (in source) and
> `LOKLIK_CRAFTER_2` (in the FlashTool path); both ship as the **iCraft**.
