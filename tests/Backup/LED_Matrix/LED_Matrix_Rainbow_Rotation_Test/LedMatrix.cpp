
// LedMatrix.cpp
#include "LedMatrix.h"

namespace LedMatrix {
  CRGB leds[NUM_LEDS];

  // Internal state
  static CRGBPalette16 currentPalette = PartyColors_p; // default
  static uint32_t lastFrameMs         = 0;             // last frame time
  static uint32_t timebase            = 0;             // evolving time (noise Z)

  // Helpers
  static inline uint16_t blockBaseIndex(uint16_t column) {
    // Column is 0..31; each column is 8 LEDs contiguous along the data path
    return column * 8;
  }
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

void LedMatrix::setPalette(const CRGBPalette16& pal) {
  currentPalette = pal;
}

// Compute one color per column using 2D-ish noise sampling:
// - X dimension: column index scaled by COLUMN_NOISE_SCALE.
// - Z dimension (time): evolves each frame by TIME_NOISE_SPEED.
// We also modulate a secondary “swirl” using beatsin8 to add organic motion.
void LedMatrix::updatePartyMarbleColumns() {
  const uint32_t now = millis();
  if (now - lastFrameMs < FRAME_INTERVAL_MS) return; // non-blocking pacing
  lastFrameMs = now;

  // Advance timebase for evolving pattern
  timebase += TIME_NOISE_SPEED; // increase for faster evolution

  // Gentle oscillation to add swirl to the columns (moves the sampling point)
  const uint8_t swirlA = beatsin8(7, 0, 255);  // slow wave
  const uint8_t swirlB = beatsin8(13, 0, 255); // another wave for variation

  // Render one color per column (8 LEDs each)
  for (uint16_t col = 0; col < MATRIX_WIDTH; ++col) {
    // Spatial sample: scale column index into noise domain
    // Use 32-bit for intermediate to avoid overflow when multiplying
    const uint32_t x = (uint32_t)col * COLUMN_NOISE_SCALE;

    // Combine time and swirl offsets for richer motion
    const uint16_t z = (uint16_t)(timebase + (swirlA / 2) + (swirlB / 3));

    // Sample noise: returns 0..255
    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);

    // Map noise to palette index; widen contrast a bit
    // You can tweak the scale/offset for different “lava” feels
    uint8_t paletteIndex = qadd8(n, 30); // shift up slightly for brighter tones

    // Get the column color from the palette
    CRGB columnColor = ColorFromPalette(currentPalette, paletteIndex, 255 /*brightness*/);

    // Apply the same color to all 8 LEDs in this column
    const uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = columnColor;
    }
  }

  FastLED.show();
}
