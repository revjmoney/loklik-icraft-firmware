# FluidNC firmware for the iCraft ESP32

Pre-built **[FluidNC](https://github.com/bdring/FluidNC) v4.0.3** images for the
plain **ESP32** (the iCraft's controller is an `ESP32-D0WD-V3`). Flashing these
replaces the stock `Grbl_ESP32` firmware with FluidNC, which is configured by a
YAML file (no recompile) and adds a WiFi web UI.

> **Why this exists:** the iCraft runs an unlocked open-source stack. FluidNC is
> the modern successor to `Grbl_ESP32` — same ESP32, friendlier config. This was
> flashed to a real iCraft board; see [`../FLASH_FLUIDNC.md`](../FLASH_FLUIDNC.md)
> for the full walkthrough and [`../machines/icraft-bareboard-nohoming.yaml`](../machines/icraft-bareboard-nohoming.yaml)
> for the matching machine config.

## `v4.0.3-esp32/` — flash these

| File | Flash offset | What it is |
|---|---|---|
| `bootloader.bin` | `0x1000`   | 2nd-stage bootloader |
| `partitions.bin` | `0x8000`   | partition table |
| `boot_app0.bin`  | `0xe000`   | OTA boot selector |
| `firmware.bin`   | `0x10000`  | FluidNC application |
| `littlefs.bin`   | `0x3d0000` | filesystem (web UI + your `config.yaml`) |

## Provenance & license

These are the **official, unmodified** FluidNC v4.0.3 release binaries — the
`esp32` **wifi** build, extracted from `fluidnc-v4.0.3-win64.zip` at
<https://github.com/bdring/FluidNC/releases/tag/v4.0.3>. FluidNC is **GPLv3**;
full source is in its repository above.

```
sha256:
  bootloader.bin  5b1b0cd6bcbfd6de6d388652a46e9b6e341f36adb9127d1e654d21dadeae74bd
  partitions.bin  1ae446228d79cf83a4b83de41c33d1e01dc21a3557149d7da90fbcbfc303311d
  boot_app0.bin   f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0
  firmware.bin    e8d042e98955c557666853875244993af47a28a3461626d90c548c01e7da72bb
  littlefs.bin    686c0f3412df5fd2ae78386933f6ded5b79a586373a3995ea7d9673ea1b9e8d4
```

## `tools/`

Small serial helpers used in the flashing guide (no WiFi required):

- `fnc_upload.py` — upload a config file to FluidNC over USB serial via XMODEM
  (`$Xmodem/Receive=`). Needs `pip install pyserial xmodem`.
- `fnc_serial.py` — minimal serial console: send commands / read boot log without
  resetting the board. Needs `pip install pyserial`.

Both default to `COM19` — edit the `PORT` line for your port.
