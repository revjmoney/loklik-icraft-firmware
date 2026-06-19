#!/usr/bin/env python3
"""Tiny non-interactive FluidNC serial helper.
Usage:
  fnc_serial.py read <seconds>                 # just listen
  fnc_serial.py cmd <seconds> "<line>" ["<line>"...]  # send line(s), then listen
Opens COM19 @ 115200, toggles DTR/RTS low so it does NOT reset FluidNC.
"""
import sys, time, serial

PORT = "COM19"
BAUD = 115200

def main():
    mode = sys.argv[1]
    secs = float(sys.argv[2])
    lines = sys.argv[3:]

    s = serial.Serial()
    s.port = PORT
    s.baudrate = BAUD
    s.timeout = 0.1
    # Do NOT reset the ESP32 on open (keep DTR/RTS from yanking EN/IO0)
    s.dtr = False
    s.rts = False
    s.open()
    time.sleep(0.2)

    if mode == "cmd":
        for ln in lines:
            s.write((ln + "\n").encode())
            s.flush()
            time.sleep(0.15)

    end = time.time() + secs
    buf = b""
    while time.time() < end:
        data = s.read(4096)
        if data:
            buf += data
            sys.stdout.write(data.decode("utf-8", "replace"))
            sys.stdout.flush()
    s.close()

if __name__ == "__main__":
    main()
