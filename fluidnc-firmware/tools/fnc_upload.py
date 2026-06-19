#!/usr/bin/env python3
"""Upload a file to FluidNC LocalFS over USB serial via XMODEM.
Replicates fluidterm's $Xmodem/Receive flow. No WiFi needed.
Usage: fnc_upload.py <localfile> <destname>
"""
import sys, time, serial
from xmodem import XMODEM

PORT, BAUD = "COM19", 115200
localfile, destname = sys.argv[1], sys.argv[2]

s = serial.Serial()
s.port, s.baudrate, s.timeout = PORT, BAUD, 1
s.dtr = False           # don't reset the ESP32 on open
s.rts = False
s.open()
time.sleep(0.3)
s.reset_input_buffer()

# Put FluidNC into receive mode
s.write(f"$Xmodem/Receive={destname}\n".encode())
s.flush()
time.sleep(0.4)

def getc(size, timeout=1):
    s.timeout = timeout
    return s.read(size) or None

def putc(data, timeout=1):
    return s.write(data) or None

stream = open(localfile, "rb")
modem = XMODEM(getc, putc, mode="xmodem")
ok = modem.send(stream, retry=16, timeout=3)
stream.close()
print("\nXMODEM send result:", "OK" if ok else "FAILED")

# drain trailing messages
s.timeout = 0.2
end = time.time() + 3
while time.time() < end:
    d = s.read(4096)
    if d:
        sys.stdout.write(d.decode("utf-8", "replace"))
        sys.stdout.flush()
s.close()
