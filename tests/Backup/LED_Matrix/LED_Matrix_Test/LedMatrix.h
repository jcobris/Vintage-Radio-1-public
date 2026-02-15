
// LedMatrix.h
#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <FastLED.h>

namespace LedMatrix {
  // ================== User-configurable settings ==================
  static const uint8_t DATA_PIN = 6;             // Data pin to WS2812B DIN
  static const uint16_t MATRIX_WIDTH  = 32;      // columns
  static const uint16_t MATRIX_HEIGHT = 8;       // rows
  static const uint16_t NUM_LEDS = MATRIX_WIDTH * MATRIX_HEIGHT; // 256

  // Color order; WS2812B is typically GRB
  static const EOrder COLOR_ORDER = GRB;

  // Brightness cap (global)
  static const uint8_t BRIGHTNESS = 50; // 0-255

  // Optional layout flags (not used by blocks function but kept for future features)
  static const bool SERPENTINE = true;   // set to false if all rows are left->right
  static const bool ROW0_AT_TOP = true;  // flip if your first row is at the bottom

  // LED buffer
  extern CRGB leds[NUM_LEDS];

  // Initialize FastLED
  void begin();

  // Clear all LEDs
  void clear();

  // Convert (x, y) to linear index (for future use)
  uint16_t xyToIndex(uint16_t x, uint16_t y);

  // === New: Rainbow in 32 blocks (8 LEDs per block) ===
  // Each consecutive group of 8 LEDs shares one hue; spans 0..255 across 32 blocks.
  void setRainbowBlocks8(uint8_t hueOffset = 0, uint8_t sat = 255, uint8_t val = 255);
}

#endif // LEDMATRIX_H
