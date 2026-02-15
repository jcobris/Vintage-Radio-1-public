
// LED_Matrix_4Folder.ino
// Mode-driven matrix controller for vintage radio project.
// Non-blocking. One color per column (8 LEDs per column).
// Folder mapping:
//  - 1: Party marble rainbow (smooth & relaxed)
//  - 2: Fire (flickery, fiery columns with visible heat-modulated glow)
//  - 3: Christmas (half red / half green, calm alternating pulse)
//  - 4: Spooky (aqua/green, slow breathing pulse, favor green)
//  - 99: Dead space (matrix off)

#include "LedMatrix.h"

// --- Simulated inputs (replace later with your tuning RC & source selector) ---
int  folder         = 2;     // 1..4 or 99 (off mode)
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
  // Update LEDs based on current folder & on/off flag (non-blocking)
  LedMatrix::update((uint8_t)folder, lights_on_off);

  // Example auto-cycler (OPTIONAL, comment out for manual testing):
  // static uint32_t t0 = millis();
  // if (millis() - t0 > 10000) { folder = (folder == 4 ? 99 : folder + 1); t0 = millis(); }
}