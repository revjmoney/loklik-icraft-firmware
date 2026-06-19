// sensor_probe_firmware.ino - minimal ESP32 sketch to stream the iCraft
// registration + pen/home sensors to a PC over USB serial.
//
// This is a STANDALONE bench tool: flash it to ANY ESP32 (the stock iCraft
// board, or a spare dev board) with the sensors wired up, and it dumps live
// readings you can watch from read_sensors.py. It is NOT FluidNC - FluidNC
// can't analogRead the optical eye, which is exactly why this exists.
//
// Wiring (matches the stock iCraft, but any pins work - edit below):
//   GPIO35  optical registration eye  (analog, ADC1_CH7)   <- the "blue light"
//   GPIO39  pen-1 endstop             (digital, active HIGH)
//   GPIO34  pen-2 endstop             (digital, active HIGH)
//   GPIO25  X home switch             (digital, active LOW)
//   GPIO26  paper/mat present         (digital, active LOW)
//
// NOTE: GPIO34-39 are input-only with NO internal pull-ups. On the stock board
// the pulls are external; on a bare dev board add your own (or expect floating
// reads on pen1/pen2).
//
// Output: one CSV line per sample at ~50 Hz:
//   millis,optical,pen1,pen2,xhome,paper

const int PIN_OPTICAL = 35;   // analog
const int PIN_PEN1    = 39;
const int PIN_PEN2    = 34;
const int PIN_XHOME   = 25;
const int PIN_PAPER   = 26;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);                          // 0..4095
  analogSetPinAttenuation(PIN_OPTICAL, ADC_11db);    // full ~0..3.3V swing
  pinMode(PIN_PEN1, INPUT);
  pinMode(PIN_PEN2, INPUT);
  pinMode(PIN_XHOME, INPUT_PULLUP);
  pinMode(PIN_PAPER, INPUT_PULLUP);
  Serial.println("# millis,optical,pen1,pen2,xhome,paper");
}

void loop() {
  int optical = analogRead(PIN_OPTICAL);
  int pen1    = digitalRead(PIN_PEN1);
  int pen2    = digitalRead(PIN_PEN2);
  int xhome   = digitalRead(PIN_XHOME);
  int paper   = digitalRead(PIN_PAPER);
  Serial.printf("%lu,%d,%d,%d,%d,%d\n", millis(), optical, pen1, pen2, xhome, paper);
  delay(20);   // ~50 Hz
}
