
// LedMatrixTest.ino
// Mode-driven matrix controller for vintage radio project.
// Non-blocking. One color per column (8 LEDs per column).
// Folder mapping:
//  - 1: Party marble rainbow (smooth & relaxed)
//  - 2: Fire (flickery, fiery columns)
//  - 3: Christmas (half red / half green, calm alternating pulse)
//  - 4: Spooky (green/blue, slow breathing pulse, favor green)
//  - 99: Dead space (matrix off)

#include "LedMatrix.h"

// --- Simulated inputs (replace later with your tuning RC & source selector) ---
int  folder         = 1;     // 1..4 or 99 (off mode)
bool lights_on_off  = true;  // true = matrix ON, false = OFF

void setup() {
  Serial.begin(115200);
  delay(300); // small one-time startup pause for serial

  LedMatrix::begin();

  Serial.println(F("Vintage radio matrix controller started."));
  Serial.print(F("Initial lights_on_off = "));
  Serial.println(lights_on_off ? F("true") : F("false"));
  Serial.print(F("Initial folder = "));
  Serial.println(folder);
}

void loop() {
  // In your real build, update 'folder' and 'lights_on_off' from the tuning RC and selector.
  // For now, we just run the update on the simulated values.
  LedMatrix::update(folder, lights_on_off);

  // Example testing (OPTIONAL):
  // After 10 seconds, cycle through folders automatically:
  // static uint32_t t0 = millis();
  // if (millis() - t0 > 10000) { folder = (folder == 4 ? 99 : folder + 1); t0 = millis(); }
}
