
// LedMatrix.cpp
#include "LedMatrix.h"

namespace LedMatrix {
  CRGB leds[NUM_LEDS];
}

using namespace LedMatrix;

void LedMatrix::begin() {
  FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  clear();
  FastLED.show();
}

void LedMatrix::clear() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

// Keep mapping helper available for matrix-based patterns later
uint16_t LedMatrix::xyToIndex(uint16_t x, uint16_t y) {
  uint16_t row = ROW0_AT_TOP ? y : (MATRIX_HEIGHT - 1 - y);
  uint16_t base = row * MATRIX_WIDTH;
  if (SERPENTINE && (row % 2 == 1)) {
    return base + (MATRIX_WIDTH - 1 - x);
  } else {
    return base + x;
  }
}

void LedMatrix::setRainbowBlocks8(uint8_t hueOffset, uint8_t sat, uint8_t val) {
  // There are 256 LEDs total; we create 32 blocks of 8 LEDs each.
  const uint16_t BLOCK_SIZE = 8;              // LEDs per block
  const uint16_t NUM_BLOCKS = NUM_LEDS / BLOCK_SIZE; // 32

  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    uint16_t block = i / BLOCK_SIZE;          // 0..31
    // Map block 0..(NUM_BLOCKS-1) to hue 0..255
    uint8_t hue = (uint8_t)((NUM_BLOCKS > 1) ? ((block * 255) / (NUM_BLOCKS - 1)) : 0);
    hue = hue + hueOffset;                    // optional drift
    leds[i] = CHSV(hue, sat, val);
  }
  FastLED.show();
}
