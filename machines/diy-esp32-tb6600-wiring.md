# ESP32 → TB6600 wiring (common-anode)

One TB6600 per axis. Repeat for X, Y, Z. Pins match `esp32-tb6600-config.yaml`.

## Per-driver connections

```
  ESP32 (3.3V logic)                 TB6600
  ------------------                 ------
  3.3V ───────┬────────────────────► PUL+
              ├────────────────────► DIR+
              └────────────────────► ENA+        (tie all three "+" together)

  STEP gpio ──────────────────────► PUL-
  DIR  gpio ──────────────────────► DIR-
                                     ENA-  →  leave OPEN  (motor always enabled)

  GND ────────┬────────────────────► (logic ground reference)
              │
              └────────────────────► motor-PSU (−)   ◄── COMMON GROUND (required!)

  Motor PSU (+24..48V) ───────────► VCC
  Motor PSU (−) ──────────────────► GND
  Stepper coils ──────────────────► A+  A−   B+  B−
```

## Pin map (from the config)

| Axis | STEP → PUL- | DIR → DIR- | Limit switch |
|------|-------------|------------|--------------|
| X    | GPIO 16     | GPIO 17    | GPIO 25 → GND |
| Y    | GPIO 18     | GPIO 19    | GPIO 26 → GND |
| Z    | GPIO 21     | GPIO 22    | GPIO 27 → GND |

All STEP/DIR `+` lines → ESP32 **3.3V**. All `ENA` left disconnected.

## The 4 things that make-or-break it

1. **Common ground.** ESP32 GND ↔ motor-PSU GND. Without it, nothing steps.
2. **Tie the "+" rail to 3.3V**, not 5V. (Pin idles HIGH = opto off; config inverts
   step with `:low` so a pulse pulls it LOW = opto on = clean step.)
3. **DIP switches on each TB6600:**
   - *Microstep* DIPs → set to **16** (or match whatever you put in `steps_per_mm`).
   - *Current* DIPs → set to your motor's **rated amps** (start at ~70-80% of rated).
   - Use the table printed on the driver's label.
4. **Two power domains.** ESP32 from USB/5V; motors from a 24-48V PSU. Don't feed
   motor voltage into the ESP32.

## Bring-up order (don't skip)

1. Flash FluidNC, upload `esp32-tb6600-config.yaml`, open the WebUI/console.
2. Power ESP32 only (no motor PSU yet). Confirm it boots and reads the config.
3. Add motor PSU. Jog X a few mm. Check direction — if backwards, add `:low` to
   that axis's `direction_pin` (or swap one coil pair).
4. Calibrate steps/mm: command 100 mm, measure actual, adjust:
   `new = old × (commanded / measured)`.
5. Add/test limit switches, then set `must_home: true`.
