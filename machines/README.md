# Machine definitions

Everything you need to rebuild, repair, or convert a LOKLiK iCraft — derived from
the factory firmware source.

| File | What it is |
|---|---|
| [`icraft-pinmap-and-tune.md`](icraft-pinmap-and-tune.md) | The factory pin map, TMC2209 settings, and motion tune (steps/mm, rates, accel, homing) pulled from `src/Device/SJMachine.h`. Includes the equivalent grbl `$$` settings. |
| [`icraft-fluidnc-config.yaml`](icraft-fluidnc-config.yaml) | Ready-to-use **FluidNC** config reproducing the factory tune. ESP32 only. |
| [`marlin-cutter-config.md`](marlin-cutter-config.md) | Notes for running the machine on **Marlin** (e.g. a Creality 4.2.7 / STM32 board). |

## Which firmware should I use?

| Route | Board | Firmware | Notes |
|---|---|---|---|
| Stay stock-but-unlocked | the iCraft's own ESP32 | this repo's Grbl_ESP32 | flash a clean image, talk G-code |
| Upgrade | any ESP32 | **FluidNC** | YAML config, no recompile; use `icraft-fluidnc-config.yaml` |
| Reuse a printer board | Creality 4.2.7 (**STM32**) | **Marlin** | FluidNC will NOT run on STM32 |
| PC-based | breakout board | Mach3/grbl | older; overkill for a cutter |

> The factory machine names are `LOKLIK_CRAFTER_1` (in source) and
> `LOKLIK_CRAFTER_2` (in the FlashTool path); both ship as the **iCraft**.
