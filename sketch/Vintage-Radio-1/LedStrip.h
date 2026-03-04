// LedStrip.h
#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "Config.h"

/*
 ============================================================
 WS2812B LED Strip (64 LEDs) - Lightweight Module
 ============================================================

 Goals:
 - Minimal SRAM/CPU impact (Nano-friendly)
 - Clean API so folder -> theme mapping can be added later
 - Avoid FastLED.show() duplication:
   The main loop should call LedStrip::update() BEFORE LedMatrix::update().
   LedMatrix::update() already calls FastLED.show(), which updates ALL controllers.

 Current behaviour:
 - Simple test pattern (rainbow) when lightsOn==true and folder != 99
 - Off (cleared) when lightsOn==false OR folder==99
*/

namespace LedStrip {
  // Initialize FastLED controller for the strip and clear it.
  void begin();

  // Non-blocking update (does NOT call FastLED.show()).
  // folder: 1..4 normal, 99 = off
  // lightsOn: false = off
  void update(uint8_t folder, bool lightsOn);

  // Clear the strip buffer to black (does not call FastLED.show()).
  void clear();
}