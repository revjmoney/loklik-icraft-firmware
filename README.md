# LOKLiK iCraft Firmware (mirror + documentation)

Open mirror of the **GPLv3** firmware that runs the **LOKLiK iCraft** cutting
machine — a modified build of [bdring/Grbl_Esp32](https://github.com/bdring/Grbl_Esp32) —
together with the firmware images, the machine's pin map and motion tune, and
ready-to-use FluidNC / Marlin configs for repairs and conversions.

> The iCraft's controller is an ESP32 running open-source `Grbl_ESP32` (GPLv3).
> LOKLiK ships it behind a login-gated app and discloses the source nowhere a
> customer would find it. This repo surfaces it. Full story in
> **[STORY.md](STORY.md)**; the verifiable record in **[EVIDENCE.md](EVIDENCE.md)**.

## Contents

| Path | What it is |
|---|---|
| `Grbl_Esp32/`, `libraries/`, `platformio.ini`, build scripts | Clean, buildable mirror of the LOKLiK/HKsjtech source (original commit history preserved) |
| [`machines/`](machines/) | iCraft pin map + motion tune, and **FluidNC** + **Marlin** configs |
| [`firmware-dumps/`](firmware-dumps/) | 8 FlashTool firmware versions, full 8 MB device backup, original NVS, partition tables |
| [`evidence/`](evidence/) | Raw GitHub API / Wayback data backing EVIDENCE.md |
| [`STORY.md`](STORY.md) / [`EVIDENCE.md`](EVIDENCE.md) | The backstory and the receipts |
| `UPSTREAM_README_grbl_esp32.md` | The original upstream Grbl_ESP32 README |

## Build it

```sh
# PlatformIO (recommended)
pio run -e release
# output: .pio/build/release/firmware.bin
```

Verified to build with PlatformIO using platform `espressif32@3.0.0` (board
`esp32dev`). The resulting binary reports `Grbl_ESP32 Ver 1.3a Date 20211103`,
matching the firmware on a stock iCraft. See [EVIDENCE.md](EVIDENCE.md).

To match a shipped image exactly you may want to set `MACHINE_NAME` and enable
`ENABLE_BLUETOOTH` / `ENABLE_WIFI` in `Grbl_Esp32/src/Device/SJMachine.h`.

## Flash it

The controller is an ESP32 (app partition at `0x10000`). Using `esptool`:

```sh
# back up first!
esptool.py read_flash 0 0x800000 my_backup.bin
# then write a firmware image
esptool.py write_flash 0x10000 firmware-dumps/flashtool-versions/firmware_v03.bin
```

> **Back up your device before writing anything.** You are responsible for what you
> flash to your own hardware.

## License

GPLv3, inherited from upstream Grbl_ESP32 — see [LICENSE](LICENSE). Credits in
[STORY.md](STORY.md).
