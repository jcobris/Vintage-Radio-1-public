
// Display_LED.h
#pragma once
#include <Arduino.h>

namespace DisplayLED {
  // Sets pin to OUTPUT and defaults to digital HIGH (solid ON).
  void init(uint8_t displayPin);

  // Pulse tick for folder 3 (linear fade with edge reversal), gated by interval.
  void pulseTick(uint16_t intervalMs = 35);
}
