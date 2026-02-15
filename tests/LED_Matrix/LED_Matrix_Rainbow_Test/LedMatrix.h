
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
  static const uint8_t BRIGHTNESS = 30; // Jeff set to 30 for comfort

  // Animation speed control (milliseconds between frames)
  // Lower value = faster animation. Try 8–20 for smooth motion.
  static const uint16_t FRAME_INTERVAL_MS = 12;

  // LED buffer
  extern CRGB leds[NUM_LEDS];

  // Initialize FastLED
  void begin();

  // Clear all LEDs
  void clear();

  // ===== Block-based rainbow (8 LEDs per color) =====
  // Static render with optional hue offset
  void setRainbowBlocks8(uint8_t hueOffset = 0, uint8_t sat = 255, uint8_t val = 255);

  // Non-blocking animated update; internally increments hue offset over time
  void updateRainbowBlocks8(uint8_t sat = 255, uint8_t val = 255);
}

#endif // LEDMATRIX_H
