
// LedStrip.h
#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "Config.h"

/*
============================================================
WS2812B LED Strip (128 LEDs) - Lightweight Module
============================================================
Goals:
 - Minimal SRAM/CPU impact (Nano-friendly)
 - Avoid FastLED.show() duplication:
   The main loop should call LedStrip::update() BEFORE LedMatrix::update().
   LedMatrix::update() already calls FastLED.show(), which updates ALL controllers.

Current behaviour (preserved):
 - Off (cleared/latched) when lightsOn==false OR folder==99
 - Non-blocking update, frame-throttled
 - update() does NOT call FastLED.show()

New behaviour (this task):
 - Folder-based theming:
   1: Party / rainbow marble vibe
   2: Fire vibe
   3: Christmas half pulse (left red, right green)
   4: Spooky green/aqua vibe with externally supplied shared breathing brightness

Spooky sync:
 - setSpookyBreath(pulse) lets the main sketch provide the exact sharedBreath used
   by matrix + dial LED so all three are in sync.
 - Pass 0xFF to release override (no external breath).
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

  // Provide an external breath brightness (0..255) used by Folder 4 renderer.
  // Pass 0xFF to release and use internal fallback behaviour (if any).
  void setSpookyBreath(uint8_t pulse);
} // namespace LedStrip
