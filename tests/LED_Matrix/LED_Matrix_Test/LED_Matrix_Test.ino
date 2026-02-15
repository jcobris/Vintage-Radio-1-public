
// LedMatrixTest.ino
// Rainbow in 32 blocks: every 8 LEDs share the same hue (32 columns x 8 rows).
// This uses contiguous blocks of 8 along the LED data path, matching a typical 8x32 matrix column wiring.

#include "LedMatrix.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  LedMatrix::begin();
  LedMatrix::setRainbowBlocks8();
  Serial.println(F("Rainbow set in 32 blocks (each block = 8 LEDs same hue)."));
}

void loop() {
  // Static display; nothing to do.
}

