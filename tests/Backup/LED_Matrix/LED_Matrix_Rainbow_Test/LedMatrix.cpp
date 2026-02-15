
// LedMatrix.cpp
#include "LedMatrix.h"

namespace LedMatrix {
  CRGB leds[NUM_LEDS];
  static uint8_t  hueOffset   = 0;     // animates over time
  static uint32_t lastFrameMs = 0;     // last time we updated
}

using namespace LedMatrix;

void LedMatrix::begin() {
  FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  clear();
  FastLED.show();
  lastFrameMs = millis();
}

void LedMatrix::clear() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void LedMatrix::setRainbowBlocks8(uint8_t offset, uint8_t sat, uint8_t val) {
  const uint16_t BLOCK_SIZE = 8;                     // 8 LEDs per block
  const uint16_t NUM_BLOCKS = NUM_LEDS / BLOCK_SIZE; // 32

  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    uint16_t block = i / BLOCK_SIZE;                 // 0..31
    uint8_t hue = (uint8_t)((NUM_BLOCKS > 1) ? ((block * 255) / (NUM_BLOCKS - 1)) : 0);
    hue += offset;                                   // offset for animation
    leds[i] = CHSV(hue, sat, val);
  }
  FastLED.show();
}

void LedMatrix::updateRainbowBlocks8(uint8_t sat, uint8_t val) {
  const uint32_t now = millis();
  if (now - lastFrameMs < FRAME_INTERVAL_MS) return; // non-blocking pacing
  lastFrameMs = now;

  // advance hue offset for gentle drift; tweak step for speed
  hueOffset += 2; // try 1..5; higher = faster hue cycle

  setRainbowBlocks8(hueOffset, sat, val);
}
