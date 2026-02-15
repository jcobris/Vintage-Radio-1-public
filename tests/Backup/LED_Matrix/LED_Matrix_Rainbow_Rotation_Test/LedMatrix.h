
// LedMatrix.h
#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <FastLED.h>

namespace LedMatrix {
  // ================== User-configurable settings ==================
  static const uint8_t  DATA_PIN          = 6;    // WS2812B DIN (through 330–470Ω recommended)
  static const uint16_t MATRIX_WIDTH      = 32;   // columns
  static const uint16_t MATRIX_HEIGHT     = 8;    // rows
  static const uint16_t NUM_LEDS          = MATRIX_WIDTH * MATRIX_HEIGHT; // 256

  // WS2812B is typically GRB
  static const EOrder   COLOR_ORDER       = GRB;

  // Brightness cap (global) — you prefer 30
  static const uint8_t  BRIGHTNESS        = 50;

  // Animation pacing (milliseconds between frames)
  // Lower value = faster motion. Try 8–16 for vivid movement.
  static const uint16_t FRAME_INTERVAL_MS = 15;

  // Noise parameters (tweak to taste)
  // Scaling affects spatial “granularity” of the marbling
  static const uint16_t COLUMN_NOISE_SCALE = 4000; // larger = smoother gradient between columns
  // Time speed affects how quickly the colors evolve
  static const uint16_t TIME_NOISE_SPEED   = 3;   // larger = faster evolution

  // LED buffer
  extern CRGB leds[NUM_LEDS];

  // Initialize FastLED
  void begin();

  // Clear all LEDs
  void clear();

  // ==== Party marble columns (lava lamp) ====
  // Change palette at runtime (RainbowColors_p, PartyColors_p, OceanColors_p, etc.)
  void setPalette(const CRGBPalette16& pal);

  // Non-blocking animated update; computes one color per column via noise+palette
  void updatePartyMarbleColumns();
}

#endif // LEDMATRIX_H
