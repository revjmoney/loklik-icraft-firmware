# LOKLiK iCraft Firmware (mirror + documentation)

Open mirror of the **GPLv3** firmware that runs the **LOKLiK iCraft** cutting
machine — a modified build of [bdring/Grbl_Esp32](https://github.com/bdring/Grbl_Esp32) —
together with the firmware images, the machine's pin map and motion tune, and
ready-to-use FluidNC / Marlin configs for repairs and conversions.

> The iCraft's controller is an ESP32 running open-source `Grbl_ESP32` (GPLv3).
> LOKLiK ships it behind a login-gated app and discloses the source nowhere a
> customer would find it. This repo surfaces it. Full story in
> **[STORY.md](STORY.md)**; the verifiable record in **[EVIDENCE.md](EVIDENCE.md)**.

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

These are mapped **but disabled** in the configs (`must_home: false`, `hard_limits: false`) for safe bring-up — not because they're unknown.

**What we still need — this is where you come in:**
- **Mat / material-presence sensor** and **tool-depth sensor** — not in the factory pin list; trace them with a multimeter / logic analyzer.
- **Confirm or repair the X IR home sensor** so X homing can be re-enabled.
- Any other limit / sensor inputs on your specific iCraft revision.

### How to help
- **Trace it** — meter the sensor/switch back to an ESP32 GPIO.
- **Submit a PR** — add a sensor-enabled YAML to the [`machines/`](machines/) folder.
- **Open an issue** — report bugs or share machine-behavior insights.

---

## Contents

| Path | What it is |
|---|---|
| `Grbl_Esp32/`, `libraries/`, `platformio.ini`, build scripts | Clean, buildable mirror of the LOKLiK/HKsjtech source (original commit history preserved) |
| [`machines/`](machines/) | iCraft pin map + motion tune, and **FluidNC** + **Marlin** configs |
| [`fluidnc-firmware/`](fluidnc-firmware/) | Pre-built **FluidNC v4.0.3** ESP32 images + upload tools — flash these to convert the iCraft to FluidNC |
| [`tools/kilkol-knife/`](tools/kilkol-knife/) | **KilKol Knife** — turn LightBurn g-code into drag-knife / pen g-code (blade-offset comp + overcut) |
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

## Convert it to FluidNC

[FluidNC](https://github.com/bdring/FluidNC) is the modern successor to
`Grbl_ESP32` — same ESP32, configured by a YAML file (no recompile) with a WiFi
web UI. Pre-built v4.0.3 ESP32 images are in
[`fluidnc-firmware/`](fluidnc-firmware/), and the matching iCraft config (factory
motion tune, no homing/sensors) is
[`machines/icraft-bareboard-nohoming.yaml`](machines/icraft-bareboard-nohoming.yaml).

Rebuilding on a **fresh ESP32 + external TB6600 drivers** (e.g. after a fried
factory board)? Use the field-tuned
[`machines/icraft-wroom-tb6600-tuned.yaml`](machines/icraft-wroom-tb6600-tuned.yaml)
— every value confirmed on the bench, with the real-world gotchas (X/Y swap,
direction invert, Z idle-release for a hot blade motor) documented inline.

**Full step-by-step (backup → erase → flash → config → WiFi): [FLASH_FLUIDNC.md](FLASH_FLUIDNC.md).**

Short version:

```sh
# 0. back up stock first!
python -m esptool --port COM19 --baud 921600 read-flash 0 ALL stock_backup.bin
# 1. erase
python -m esptool --chip esp32 --port COM19 --baud 921600 erase-flash
# 2. flash firmware + 3. filesystem  (see FLASH_FLUIDNC.md for the full commands)
#    bootloader@0x1000  partitions@0x8000  boot_app0@0xe000  firmware@0x10000  littlefs@0x3d0000
# 4. upload the machine config over USB serial (XMODEM)
python fluidnc-firmware/tools/fnc_upload.py machines/icraft-bareboard-nohoming.yaml config.yaml
```

## Cut with LightBurn → **KilKol Knife**

Once it's on FluidNC, the iCraft speaks plain G-code, so you can drive it with
whatever you like. If you already own **LightBurn**, you can keep designing in it
and still run the **drag knife** — even though LightBurn is a laser program.

[`tools/kilkol-knife/`](tools/kilkol-knife/) is a tiny, dependency-free Python
app (GUI **and** CLI) that **converts a normal LightBurn "Save GCode" file into
drag-knife / pen G-code**. LightBurn already splits jobs into *travel* (laser
off) and *cut* (laser on) — KilKol Knife turns that into **blade up / blade
down**, strips the laser commands, and respects the iCraft's `Z0 = up` convention.

```sh
# pick a material preset and go:
python tools/kilkol-knife/kilkol_knife.py  myjob.gc  myjob_pen.gc  --material chrome
#   ...or run it with no args for the GUI.
```

It's **inch‑native** (outputs `G20`, so FluidNC + the CYD pendant stay in inches),
does real drag‑knife **blade‑offset compensation + overcut** (sharp corners),
**multi‑pass / two‑cut trick** (`--passes`, for thick chrome/holographic), and
**material presets** (`--material chrome|vinyl|htv|holographic`). Blade‑offset math
ported from [Inkcut](https://github.com/codelv/inkcut) (GPLv3). Full usage in
[`tools/kilkol-knife/README.md`](tools/kilkol-knife/README.md); the blade‑depth /
pressure / blade‑angle / spring‑calibration guide is in
[`tools/kilkol-knife/TUNING.md`](tools/kilkol-knife/TUNING.md).

So the whole pipeline is: **design in LightBurn → KilKol Knife → gSender →
vinyl.** No cloud, no login, no proprietary app.

---

## 📟 A note for LOKLiK

You built a cutter on **GPLv3** software (`Grbl_ESP32`), shipped it locked behind
a login wall, and pointed paying customers to **exactly nowhere** for the source
the license obligates you to provide. The GPL isn't a vibe — **Section 6** says
the source travels *with* the binary. So treat this repo as your compliance
department: the code you were supposed to publish, finally published — plus the
pin map, the flashing steps, and a config that turns your "appliance" back into
**a real, open CNC machine** that needs nothing from you.

I got a full refund. The community got the source. And this iCraft now runs with
**no cloud, no login, and no LOKLiK** anywhere in the loop.

Funny how open hardware wants to be free. 😉

— *Rev. J. Money · [jmscnc.com](https://jmscnc.com)*

---

## License

GPLv3, inherited from upstream Grbl_ESP32 — see [LICENSE](LICENSE). Credits in
[STORY.md](STORY.md). **KilKol Knife** is also GPLv3 (blade-offset comp derived
from Inkcut, © Jairus Martin).
