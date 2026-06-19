# Flashing FluidNC onto the iCraft ESP32

> 🚨 **Call for help:** the bare config below runs motors + buttons only. The limit/home
> pins are already known (X `GPIO25` — often dead, Z `GPIO39`, Y none), but the **mat /
> material sensor** and **tool-depth sensor** still need tracing. Help map them — PR a
> sensor-enabled YAML to [`machines/`](machines/) or open an issue. Full map + details in
> the [main README](README.md#-call-for-community-help--contributions).

Replace the stock `Grbl_ESP32` firmware with **FluidNC v4.0.3** using `esptool`.
This was done on a real iCraft board (`ESP32-D0WD-V3`, 8 MB flash). FluidNC is
then configured with a YAML file — see
[`machines/icraft-bareboard-nohoming.yaml`](machines/icraft-bareboard-nohoming.yaml)
(no homing / no sensors — motors + buttons only, the factory motion tune).

> ⚠️ **This erases the stock firmware. Back it up first (Step 0) — it is the only
> way back.** You are responsible for what you flash to your own hardware.

## Requirements

```sh
pip install esptool pyserial xmodem
```

- A USB cable to the board. Note its serial port (Windows `COMx`; Linux/macOS
  `/dev/ttyUSB0` etc.). Examples below use `COM19` — substitute yours.
- The firmware images in [`fluidnc-firmware/v4.0.3-esp32/`](fluidnc-firmware/v4.0.3-esp32/).

Confirm the board responds (read-only):

```sh
python -m esptool --port COM19 chip-id
# expect: Chip type ... ESP32-D0WD-V3 ... and a MAC address
```

## Step 0 — Back up the stock firmware (do NOT skip)

```sh
python -m esptool --port COM19 --baud 921600 read-flash 0 ALL stock_backup.bin
```

This pulls the full 8 MB image. Keep it safe — it is your restore point.
If the machine still runs stock GRBL, also capture its settings from a GRBL
sender: run `$$` and save the output (steps/mm, rates, accel, etc.).

## Step 1 — Erase

```sh
python -m esptool --chip esp32 --port COM19 --baud 921600 erase-flash
```

## Step 2 — Flash the FluidNC firmware

Run from the repo root (paths point into `fluidnc-firmware/v4.0.3-esp32/`):

```sh
python -m esptool --chip esp32 --port COM19 --baud 921600 \
  --before default-reset --after hard-reset write-flash -z \
  --flash-mode dio --flash-freq 80m --flash-size detect \
  0x1000  fluidnc-firmware/v4.0.3-esp32/bootloader.bin \
  0x8000  fluidnc-firmware/v4.0.3-esp32/partitions.bin \
  0xe000  fluidnc-firmware/v4.0.3-esp32/boot_app0.bin \
  0x10000 fluidnc-firmware/v4.0.3-esp32/firmware.bin
```

## Step 3 — Flash the filesystem (web UI + config storage)

```sh
python -m esptool --chip esp32 --port COM19 --baud 921600 \
  --before default-reset --after hard-reset write-flash -z \
  --flash-mode dio --flash-freq 80m --flash-size detect \
  0x3d0000 fluidnc-firmware/v4.0.3-esp32/littlefs.bin
```

## Step 4 — Confirm it boots

Open a serial terminal at **115200 baud** (e.g. `fluidnc-firmware/tools/fnc_serial.py`,
or any terminal) and send `$I`. You should see:

```
[VER:4.0 FluidNC v4.0.3 (esp32-wifi) :]
[MSG:Machine: Default (Test Drive no I/O)]
```

It boots the default machine until you upload a config (next step).

## Step 5 — Upload the machine config

Send the YAML to the board's filesystem as `config.yaml`.

**Over USB serial (no WiFi)** — uses XMODEM, the way FluidNC's own tool does it:

```sh
python fluidnc-firmware/tools/fnc_upload.py machines/icraft-bareboard-nohoming.yaml config.yaml
# -> [MSG:INFO: Received NNNN bytes to file /littlefs/config.yaml]
```

**Or over WiFi:** FluidNC boots a `FluidNC` access point at `192.168.0.1`. Connect
to it, open the web UI, and upload the YAML in the file browser.

Then restart and verify a clean parse:

```sh
python fluidnc-firmware/tools/fnc_serial.py cmd 9 "$Bye"
# look for: [MSG:INFO: Machine LOKLiK iCraft ...] and NO [MSG:ERR ...] lines
```

## Step 6 — (optional) Join your WiFi

```sh
python fluidnc-firmware/tools/fnc_serial.py cmd 4 "$Sta/SSID=YOURSSID" "$Sta/Password=YOURPASS" "$WiFi/Mode=STA"
python fluidnc-firmware/tools/fnc_serial.py cmd 12 "$Bye"
# watch for: [MSG:INFO: Connected - IP is 192.168.x.x]
```

Then browse to that IP (or `http://fluidnc.local`) for the full web UI: jogging,
config editing, sending G-code.

## Bring-up

With motors wired, bring axes up **one at a time**: clear any alarm (`$X`), jog a
few mm, check direction, then calibrate steps/mm against a ruler before trusting
distances. Watch driver temperature.

## Restore the stock firmware

```sh
python -m esptool --port COM19 --baud 921600 write-flash 0x0 stock_backup.bin
```

## FluidNC YAML gotchas (learned the hard way)

- **No inline `#` comments after a value** — FluidNC's parser rejects them
  (`Expected a float/integer`). Put comments on their own lines only. ASCII only.
- `step_pin` / `direction_pin` live **inside** the `tmc_2209:` (or `stepstick:`)
  block, not directly under `motor0`.
- Invert a direction with a `:low` modifier on the pin
  (`direction_pin: gpio.32:low`) — there is no `direction_invert` key.
- Input-only pins (ESP32 GPIO34–39) have **no internal pull** — a control/limit
  pin there floats and can false-trigger; needs an external pull-up.

## Config schema note

YAML schema can change between FluidNC versions. These configs target **v4.0.3**.
Validate against the [FluidNC wiki](https://github.com/bdring/FluidNC/wiki) if you
flash a different release.
