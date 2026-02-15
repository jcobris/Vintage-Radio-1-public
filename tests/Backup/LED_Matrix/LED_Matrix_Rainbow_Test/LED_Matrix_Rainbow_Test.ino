
// LedMatrixTest.ino
// Non-blocking animated rainbow in 32 blocks (every 8 LEDs same hue).
// Uses millis() timing (no delay) and a tunable frame interval.

#include "LedMatrix.h"

void setup() {
  Serial.begin(115200);
  delay(300); // small one-time startup pause for serial
  LedMatrix::begin();
  Serial.println(F("Non-blocking 8-LED block rainbow animation running."));
}

void loop() {
  LedMatrix::updateRainbowBlocks8(); // advances animation based on millis()
}
