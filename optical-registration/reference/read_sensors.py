#!/usr/bin/env python3
"""
read_sensors.py - PC-side live reader for the iCraft sensors.

Pair it with sensor_probe_firmware.ino flashed to an ESP32 that has the sensors
wired up. It streams the optical eye + pen/home/paper switches and classifies
the optical reflectance into Paper / Plate / Marker using the SAME tolerance
bins COpticalAlign used - so you can watch a registration mark pass under the
eye live and confirm the sensor works before building anything on top of it.

    pip install pyserial
    python read_sensors.py COM5            # Windows
    python read_sensors.py /dev/ttyUSB0    # Linux/Mac

IMPORTANT: the stock firmware's bins are a starting point, not gospel. The
"marker" band is the HIGHEST analog value in their calibration (in their sensor
circuit a black mark reads high) - your circuit/paper/ink may differ. Watch the
raw 'optical' column first and re-tune TOLERANCE to your setup.
"""

import sys
import serial   # pip install pyserial

# (low, high, label) - lifted from COpticalAlign::findMarkByRange tolerance_range.
TOLERANCE = [
    (0,    1,    "ERR"),     # flatline -> sensor fault / disconnected
    (1,    2500, "paper"),   # white paper
    (2500, 3000, "plate"),   # cutting mat / board
    (3200, 5500, "MARKER"),  # registration mark
]


def classify(v):
    for lo, hi, label in TOLERANCE:
        if lo <= v < hi:
            return label
    return "?"


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM5"
    baud = 115200
    with serial.Serial(port, baud, timeout=1) as ser:
        print(f"reading {port} @ {baud}  (Ctrl-C to stop)")
        last = None
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line or line.startswith("#"):
                continue
            try:
                ms, optical, pen1, pen2, xhome, paper = (int(x) for x in line.split(","))
            except ValueError:
                continue
            tag = classify(optical)
            # print only when the optical class changes -> that's a mark edge
            if tag != last:
                edge = "   <-- EDGE" if last is not None else ""
                print(f"{ms:>8}ms  optical={optical:<5} [{tag:<6}]  "
                      f"pen1={pen1} pen2={pen2} xhome={xhome} paper={paper}{edge}")
                last = tag


if __name__ == "__main__":
    main()
