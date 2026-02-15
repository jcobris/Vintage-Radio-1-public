
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

  // Spooky palette (favor green, start mid-blue ~ aqua)
  static const TProgmemRGBPalette16 SpookyAquaGreen_p PROGMEM = {
    0x10C0FF, 0x20D8FF, 0x30F0FF, 0x20FFE0,
    0x10FFC0, 0x00FFA0, 0x00FF80, 0x20FF60,
    0x40FF40, 0x30E040, 0x20C050, 0x10A070,
    0x20B8D0, 0x30D8F0, 0x20E8C0, 0x20B090
  };
  static CRGBPalette16 spookyPalette = SpookyAquaGreen_p;
  static uint32_t      spookyTime    = 0;

  // Fire (global noise-driven)
  static uint32_t      fireTime      = 0; // time evolution for noise

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
  partyTime += LedMatrix::PARTY_TIME_NOISE_SPEED;

  // Optional gentle swirl via beatsin8 (subtle offsets)
  const uint8_t swirlA = beatsin8(7, 0, 255);
  const uint8_t swirlB = beatsin8(13, 0, 255);

  for (uint16_t col = 0; col < LedMatrix::MATRIX_WIDTH; ++col) {
    uint32_t x = (uint32_t)col * LedMatrix::PARTY_COLUMN_NOISE_SCALE;
    uint16_t z = (uint16_t)(partyTime + (swirlA / 2) + (swirlB / 3));

    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);
    uint8_t index = qadd8(n, 30); // slight bias to brighter tones
    CRGB color = ColorFromPalette(partyPalette, index, 255);

    const uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = color;
    }
  }
}

// Folder 2: Fire (global noise-driven fire, red-biased, fast/flickery)
static void renderFireColumns() {
  // Evolve global fire time quickly (fast motion)
  fireTime += LedMatrix::FIRE_NOISE_TIME_SPEED;

  for (uint8_t col = 0; col < 32; ++col) {
    // --- Base noise index across columns + time ---
    const uint32_t x = (uint32_t)col * LedMatrix::FIRE_NOISE_COLUMN_SCALE;
    const uint16_t z = (uint16_t)fireTime;

    // Coarse fire “height” via noise
    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);

    // Apply red index bias and cap to keep in red/orange band
    int16_t idx16 = n;
    idx16 -= LedMatrix::FIRE_RED_IDX_BIAS;
    if (idx16 < 0) idx16 = 0;
    if (idx16 > LedMatrix::FIRE_PALETTE_CAP) idx16 = LedMatrix::FIRE_PALETTE_CAP;

    // --- Flicker: spatial + temporal modulation ---
    const uint8_t flickerN = inoise8(
      (uint16_t)(col * LedMatrix::FIRE_FLICKER_SPATIAL_SCALE),
      (uint16_t)(fireTime + 137) // phase offset to desync from base noise
    );
    const int8_t flickerDelta = (int8_t)map(
      flickerN, 0, 255,
      -(int16_t)LedMatrix::FIRE_FLICKER_DEPTH,
      +(int16_t)LedMatrix::FIRE_FLICKER_DEPTH
    );

    // Nudge index and brightness via flicker
    idx16 += (flickerDelta / 3); // conservative hue flicker
    if (idx16 < 0) idx16 = 0;
    if (idx16 > LedMatrix::FIRE_PALETTE_CAP) idx16 = LedMatrix::FIRE_PALETTE_CAP;

    int16_t bright16 = map(n, 0, 255,
                           LedMatrix::FIRE_FLAME_BRIGHT_MIN,
                           LedMatrix::FIRE_FLAME_BRIGHT_MAX);
    bright16 += flickerDelta; // brightness flicker
    if (bright16 < 0)   bright16 = 0;
    if (bright16 > 255) bright16 = 255;
    const uint8_t flameBright = (uint8_t)bright16;

    // Final palette index
    const uint8_t idx = (uint8_t)idx16;

    // Flame color from HeatColors_p + red/green channel bias
    CRGB flame = ColorFromPalette(HeatColors_p, idx, flameBright, LINEARBLEND);
    flame.r = qadd8(flame.r, LedMatrix::FIRE_RED_CHANNEL_BOOST);
    flame.g = qsub8(flame.g, LedMatrix::FIRE_GREEN_CHANNEL_REDUCE);

    // Rare white spark overlay (only away from edges; only when idx is already high)
    const bool edgeGuard = (col < LedMatrix::FIRE_WHITE_EDGE_GUARD) ||
                           (col > (31 - LedMatrix::FIRE_WHITE_EDGE_GUARD));
    if (!edgeGuard && idx >= LedMatrix::FIRE_WHITE_SPARK_IDX_MIN) {
      if (random8(255) < LedMatrix::FIRE_WHITE_SPARK_CHANCE) {
        // blend toward white modestly; still red-biased overall
        nblend(flame, CRGB::White, 100);
      }
    }

    // Optional subtle background glow under fire (kept very low)
    if (LedMatrix::FIRE_GLOW_ENABLE) {
      const uint8_t glowVal = (uint8_t)map(n, 0, 255,
                                           LedMatrix::FIRE_GLOW_MIN_VAL,
                                           LedMatrix::FIRE_GLOW_MAX_VAL);
      const CHSV glowHSV(LedMatrix::FIRE_GLOW_HUE,
                         LedMatrix::FIRE_GLOW_SAT,
                         glowVal);
      const CRGB glow = glowHSV;

      // Blend: more flame even at low; flame dominates at high
      const uint8_t eased = ease8InOutApprox(n); // use n as “heat proxy”
      uint8_t blendAmt = (uint8_t)map(eased, 0, 255,
                                      LedMatrix::FIRE_BLEND_MIN_AMOUNT,
                                      LedMatrix::FIRE_BLEND_MAX_AMOUNT);

      const uint16_t base = blockBaseIndex(col);
      for (uint8_t i = 0; i < 8; ++i) {
        leds[base + i] = blend(glow, flame, blendAmt);
      }
    } else {
      // No glow: set the flame color directly per column
      const uint16_t base = blockBaseIndex(col);
      for (uint8_t i = 0; i < 8; ++i) {
        leds[base + i] = flame;
      }
    }
  }
}

// Folder 3: Christmas calm alternating pulse
static void renderXmasColumns() {
  // Alternating pulse using beatsin8. One half bright while the other is dim, then swap.
  uint8_t brightA = beatsin8(LedMatrix::XMAS_PULSE_BPM, LedMatrix::XMAS_MIN_BRIGHT, LedMatrix::XMAS_MAX_BRIGHT, 0,   0);
  uint8_t brightB = beatsin8(LedMatrix::XMAS_PULSE_BPM, LedMatrix::XMAS_MIN_BRIGHT, LedMatrix::XMAS_MAX_BRIGHT, 0, 128);

  // Left half (0..15) red, right half (16..31) green
  for (uint16_t col = 0; col < 32; ++col) {
    CRGB color;
    if (col < 16) {
      color = CRGB(brightA, 0, 0); // red with pulsing brightness
    } else {
      color = CRGB(0, brightB, 0); // green with opposite pulsing
    }
    const uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = color;
    }
  }
}

// Folder 4: Spooky aqua/green breathing (favor green), column-uniform
static void renderSpookyColumns() {
  // Evolve time gently
  spookyTime += LedMatrix::SPOOKY_TIME_NOISE_SPEED;

  // Slow breathing pulse (value modulation)
  uint8_t pulse = beatsin8(LedMatrix::SPOOKY_PULSE_BPM, 60, 255); // breathing brightness 60..255

  for (uint16_t col = 0; col < 32; ++col) {
    uint32_t x = (uint32_t)col * LedMatrix::SPOOKY_COLUMN_NOISE_SCALE;
    uint16_t z = (uint16_t)(spookyTime);

    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);
    // Gentler bias keeps selection in mid aqua/green without over-brightening
    uint8_t index = qadd8(n, 28);

    CRGB color = ColorFromPalette(spookyPalette, index, 255);
    // Apply breathing pulse to value (brightness)
    color.nscale8_video(pulse);

    const uint16_t base = blockBaseIndex(col);
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
    lastFrameMs = now; // reset timing to avoid catch-up
    return;
  }

  // Frame pacing (non-blocking)
  if (now - lastFrameMs < LedMatrix::FRAME_INTERVAL_MS) return;
  lastFrameMs = now;

  // Dispatch by folder
  switch (folder) {
    case 1: // Party marble (smooth & relaxed)
      renderPartyMarble();
      break;

    case 2: // Fire (global noise-driven fire, red-biased, fast/flickery)
      renderFireColumns();
      break;

    case 3: // Christmas calm pulse
      renderXmasColumns();
      break;

    case 4: // Spooky aqua/green breathing
      renderSpookyColumns();
      break;

    default:
      // Unknown folder: turn off for safety
      clear();
      FastLED.show();
      return;
  }

  // Push the rendered frame to the LEDs
  FastLED.show();
}
