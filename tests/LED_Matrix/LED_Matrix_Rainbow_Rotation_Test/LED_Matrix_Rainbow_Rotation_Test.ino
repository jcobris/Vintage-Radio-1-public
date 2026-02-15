
// LedMatrixTest.ino
// Party-style marbled rainbow columns (lava-lamp-like), non-blocking.
// Each column (8 LEDs) shares one color. Uses FastLED noise + palettes.

#include "LedMatrix.h"

void setup() {
  Serial.begin(115200);
  delay(300); // small one-time startup pause for serial
  LedMatrix::begin();

  // Optional: pick your palette here
  // LedMatrix::setPalette(RainbowColors_p); // very rainbow
  LedMatrix::setPalette(PartyColors_p);     // punchy party tones

  Serial.println(F("Party marble columns running (non-blocking)."));
}

void loop() {
  LedMatrix::updatePartyMarbleColumns(); // advances animation based on millis()
}
