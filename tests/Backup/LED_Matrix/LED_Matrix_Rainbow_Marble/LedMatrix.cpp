
// LedMatrix.cpp
#include "LedMatrix.h"

// ===== Internal state =====
namespace LedMatrix {
  CRGB leds[NUM_LEDS];

  // Timing
  static uint32_t lastFrameMs = 0; // last frame time

  // Party marble state
  static CRGBPalette16 partyPalette = PartyColors_p; // or RainbowColors_p if you prefer
  static uint32_t      partyTime    = 0;

  // Spooky palette (favor green): custom
  // You can tweak these to taste (more green, some cyan/blue)
  static const TProgmemRGBPalette16 SpookyGreenBlue_p PROGMEM = {
    0x004000, 0x005000, 0x006000, 0x007000,
    0x008000, 0x00A000, 0x00C000, 0x00E000,
    0x00FF20, 0x20FF80, 0x00C080, 0x1080FF,
    0x006060, 0x004040, 0x1030A0, 0x003030
  };
  static CRGBPalette16 spookyPalette = SpookyGreenBlue_p;
  static uint32_t      spookyTime    = 0;

  // Fire columns: per-column heat (0..255)
  static uint8_t       fireHeat[32] = {0};

  // Helper: base index for a contiguous 8-LED column block
  static inline uint16_t blockBaseIndex(uint16_t column) {
    return column * 8; // columns are contiguous blocks of 8 LEDs
  }
}

// ===== Public functions =====
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

// ===== Internal helpers (renderers) =====

// Folder 1: Party marble rainbow (noise + palette), column-uniform
static void renderPartyMarble() {
  // Evolve time
  partyTime += PARTY_TIME_NOISE_SPEED;

  // Optional gentle swirl via beatsin8 (subtle offsets)
  const uint8_t swirlA = beatsin8(7, 0, 255);
  const uint8_t swirlB = beatsin8(13, 0, 255);

  for (uint16_t col = 0; col < MATRIX_WIDTH; ++col) {
    uint32_t x = (uint32_t)col * PARTY_COLUMN_NOISE_SCALE;
    uint16_t z = (uint16_t)(partyTime + (swirlA / 2) + (swirlB / 3));

    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);
    uint8_t index = qadd8(n, 30); // slight bias to brighter tones
    CRGB color = ColorFromPalette(partyPalette, index, 255);

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = color;
    }
  }
}

// Folder 2: Fire columns (flickery flames), column-uniform
static void renderFireColumns() {
  // Cool down each column slightly
  for (uint8_t col = 0; col < 32; ++col) {
    uint8_t cooldown = random8(0, ((FIRE_COOLING * 10) / 32) + FIRE_FLICKER_VARIANCE);
    fireHeat[col] = (fireHeat[col] > cooldown) ? (fireHeat[col] - cooldown) : 0;
  }

  // Random sparks near the "bottom" (conceptual; we still apply per column uniformly)
  if (random8() < FIRE_SPARKING) {
    uint8_t pos = random8(0, 32);
    fireHeat[pos] = qadd8(fireHeat[pos], random8(160, 255));
  }

  // Slight upward diffusion (smear/average neighboring columns)
  // This gives a sense of motion across columns without violating uniform column color.
  for (uint8_t col = 31; col > 0; --col) {
    fireHeat[col] = (fireHeat[col] + fireHeat[col - 1]) / 2;
  }

  // Render
  for (uint8_t col = 0; col < 32; ++col) {
    CRGB color = ColorFromPalette(HeatColors_p, fireHeat[col], FIRE_BASE_BRIGHTNESS);
    // Add little flicker variation
    if (FIRE_FLICKER_VARIANCE) {
      int8_t jitter = (int8_t)random8(0, FIRE_FLICKER_VARIANCE);
      uint8_t v = qadd8(FIRE_BASE_BRIGHTNESS, jitter);
      color.nscale8_video(v);
    }

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = color;
    }
  }
}

// Folder 3: Christmas calm alternating pulse
static void renderXmasColumns() {
  // Alternating pulse using beatsin8. One half bright while the other is dim, then swap.
  // BPM defines calm pulse speed.
  uint8_t brightA = beatsin8(LedMatrix::XMAS_PULSE_BPM, LedMatrix::XMAS_MIN_BRIGHT, LedMatrix::XMAS_MAX_BRIGHT);
  uint8_t brightB = 255 - brightA; // opposite phase

  // Left half (0..15) red, right half (16..31) green
  for (uint16_t col = 0; col < 32; ++col) {
    CRGB color;
    if (col < 16) {
      color = CRGB(brightA, 0, 0); // red with pulsing brightness
    } else {
      color = CRGB(0, brightB, 0); // green with opposite pulsing
    }
    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = color;
    }
  }
}

// Folder 4: Spooky green/blue breathing (favor green), column-uniform
static void renderSpookyColumns() {
  // Evolve time gently
  spookyTime += LedMatrix::SPOOKY_TIME_NOISE_SPEED;

  // Slow breathing pulse (value modulation)
  uint8_t pulse = beatsin8(LedMatrix::SPOOKY_PULSE_BPM, 90, 255); // breathing brightness 90..255

  for (uint16_t col = 0; col < 32; ++col) {
    uint32_t x = (uint32_t)col * LedMatrix::SPOOKY_COLUMN_NOISE_SCALE;
    uint16_t z = (uint16_t)(spookyTime);

    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);
    // Bias palette index toward green range by lifting the lower half:
    uint8_t index = qadd8(n, 40); // favor brighter/greener entries

    CRGB color = ColorFromPalette(spookyPalette, index, 255);
    // Apply breathing pulse to value (brightness)
    color.nscale8_video(pulse);

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = color;
    }
  }
}

// ===== Main update (non-blocking) =====
void LedMatrix::update(uint8_t folder, bool lightsOn) {
  const uint32_t now = millis();

  // Instant off if lights are off or folder is 99
  if (!lightsOn || folder == 99) {
    clear();
    FastLED.show();
    // Reset timing to avoid catching up with accumulated time
    lastFrameMs = now;
    return;
  }

  // Frame pacing (non-blocking)
  if (now - lastFrameMs < FRAME_INTERVAL_MS) return;
  lastFrameMs = now;

  // Dispatch by folder
  switch (folder) {
    case 1: // Party marble (smooth & relaxed)
      renderPartyMarble();
      break;

    case 2: // Fire columns
      // Optionally accelerate fire updates a bit
      for (uint8_t i = 0; i < FIRE_SPEED_SCALE; ++i) {
        renderFireColumns();
      }
      break;

    case 3: // Christmas calm pulse
      renderXmasColumns();
      break;

    case 4: // Spooky green/blue breathing
      renderSpookyColumns();
      break;

    default:
      // Unknown folder: turn off for safety
      clear();
      FastLED.show();
      break;
  }
}
