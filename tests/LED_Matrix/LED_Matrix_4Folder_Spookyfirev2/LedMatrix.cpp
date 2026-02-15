
// LedMatrix.cpp
#include "LedMatrix.h"

// ===== Internal state =====
namespace LedMatrix {
  CRGB leds[NUM_LEDS];

  // Timing
  static uint32_t lastFrameMs = 0; // last frame time

  // Party marble state
  static CRGBPalette16 partyPalette = PartyColors_p; // or RainbowColors_p if you prefer
  static uint32_t partyTime = 0;

  // Spooky palette (favor green, start mid-blue ~ aqua)
  static const TProgmemRGBPalette16 SpookyAquaGreen_p PROGMEM = {
    0x10C0FF, 0x20D8FF, 0x30F0FF, 0x20FFE0,
    0x10FFC0, 0x00FFA0, 0x00FF80, 0x20FF60,
    0x40FF40, 0x30E040, 0x20C050, 0x10A070,
    0x1080A0, 0x30A0C0, 0x20A8B0, 0x208880
  };
  static CRGBPalette16 spookyPalette = SpookyAquaGreen_p;
  static uint32_t spookyTime = 0;

  // Fire columns: per-column heat (0..255)
  static uint8_t fireHeat[32] = {0};

  // Noise time + horizontal drift for global sparks
  static uint32_t sparkNoiseTime  = 0;
  static uint32_t sparkNoiseTime2 = 0;
  static int32_t  sparkDriftX     = 0;

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
    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = color;
    }
  }
}

// Weighted average helper: mix current and neighbor with an 8-bit weight
static inline uint8_t mixWeighted(uint8_t self, uint8_t neighbor, uint8_t neighborWeight /*0..255*/) {
  // result = self*(1 - w) + neighbor*w, using 8-bit integer math
  uint16_t acc = (uint16_t)self * (255 - neighborWeight) + (uint16_t)neighbor * neighborWeight;
  return (uint8_t)(acc / 255);
}

// 8-bit mix helper for combining two noise octaves
static inline uint8_t mix8(uint8_t a, uint8_t b, uint8_t w /*0..255*/) {
  return mixWeighted(a, b, w);
}

// Folder 2: Fire columns — bidirectional diffusion + multi-octave drifting noise sparks
static void renderFireColumns() {
  // 1) Cooling — small random decay per column
  for (uint8_t col = 0; col < 32; ++col) {
    uint8_t cooldown = random8(0, ((LedMatrix::FIRE_COOLING * 10) / 32) + LedMatrix::FIRE_FLICKER_VARIANCE);
    fireHeat[col] = (fireHeat[col] > cooldown) ? (fireHeat[col] - cooldown) : 0;
  }

  // 1b) Extra edge cooling to keep ends calmer (no spark logic involved)
  for (uint8_t e = 0; e < LedMatrix::FIRE_EDGE_ATTENUATION_WIDTH; ++e) {
    uint8_t extraL = random8(0, LedMatrix::FIRE_EDGE_EXTRA_COOLING);
    uint8_t extraR = random8(0, LedMatrix::FIRE_EDGE_EXTRA_COOLING);
    fireHeat[e]           = (fireHeat[e]           > extraL) ? (fireHeat[e]           - extraL) : 0;
    fireHeat[31 - e]      = (fireHeat[31 - e]      > extraR) ? (fireHeat[31 - e]      - extraR) : 0;
  }

  // 2) Global **multi-octave** noise-driven sparks with **horizontal drift** (anywhere)
  sparkNoiseTime  += LedMatrix::FIRE_SPARK_NOISE_SPEED;
  sparkNoiseTime2 += LedMatrix::FIRE_SPARK_NOISE_SPEED2;
  sparkDriftX     += (int32_t)LedMatrix::FIRE_SPARK_DRIFT_SPEED;

  for (uint8_t col = 0; col < 32; ++col) {
    // Drift the sampling point so hotspots slowly move across the matrix
    uint16_t x1 = (uint16_t)(col * LedMatrix::FIRE_SPARK_NOISE_SCALE  + (sparkDriftX & 0xFFFF));
    uint16_t x2 = (uint16_t)(col * LedMatrix::FIRE_SPARK_NOISE_SCALE2 - ((sparkDriftX / 2) & 0xFFFF));

    // Two octaves: broad + fine detail
    uint8_t n1 = inoise8(x1, (uint16_t)sparkNoiseTime);
    uint8_t n2 = inoise8(x2, (uint16_t)sparkNoiseTime2);

    // Combine octaves (weighted mix)
    uint8_t nCombined = mix8(n1, n2, LedMatrix::FIRE_SPARK_OCTAVE_BLEND);

    // Map to a small probability range to keep sparks tasteful
    uint8_t prob = (uint8_t)map(nCombined, 0, 255, LedMatrix::FIRE_SPARK_MIN_PROB, LedMatrix::FIRE_SPARK_MAX_PROB);

    if (random8() < prob) {
      fireHeat[col] = qadd8(fireHeat[col],
                            random8(LedMatrix::FIRE_GLOBAL_SPARK_HEAT_MIN,
                                    LedMatrix::FIRE_GLOBAL_SPARK_HEAT_MAX));
    }
  }

  // 3) Bidirectional diffusion — propagate inward from both edges with partial averaging
  // Rightward smear (left -> right)
  for (uint8_t col = 31; col > 0; --col) {
    fireHeat[col] = mixWeighted(fireHeat[col], fireHeat[col - 1], LedMatrix::FIRE_DIFFUSE_WEIGHT);
  }
  // Leftward smear (right -> left)
  for (uint8_t col = 0; col < 31; ++col) {
    fireHeat[col] = mixWeighted(fireHeat[col], fireHeat[col + 1], LedMatrix::FIRE_DIFFUSE_WEIGHT);
  }

  // 3b) Edge attenuation — soft clamp after diffusion (keeps edges visually calmer)
  for (uint8_t e = 0; e < LedMatrix::FIRE_EDGE_ATTENUATION_WIDTH; ++e) {
    fireHeat[e]      = scale8(fireHeat[e],      LedMatrix::FIRE_EDGE_ATTENUATION);
    fireHeat[31 - e] = scale8(fireHeat[31 - e], LedMatrix::FIRE_EDGE_ATTENUATION);
  }

  // 4) Render — palette + brightness jitter (one color per column)
  for (uint8_t col = 0; col < 32; ++col) {
    CRGB color = ColorFromPalette(HeatColors_p, fireHeat[col], LedMatrix::FIRE_BASE_BRIGHTNESS);
    if (LedMatrix::FIRE_FLICKER_VARIANCE) {
      int8_t jitter = (int8_t)random8(0, LedMatrix::FIRE_FLICKER_VARIANCE);
      uint8_t v = qadd8(LedMatrix::FIRE_BASE_BRIGHTNESS, jitter);
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
  uint8_t brightA = beatsin8(XMAS_PULSE_BPM, XMAS_MIN_BRIGHT, XMAS_MAX_BRIGHT, 0, 0);
  uint8_t brightB = beatsin8(XMAS_PULSE_BPM, XMAS_MIN_BRIGHT, XMAS_MAX_BRIGHT, 0, 128);

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
  uint8_t pulse = beatsin8(LedMatrix::SPOOKY_PULSE_BPM, 30, 255); // breathing brightness 30..255

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
    lastFrameMs = now; // reset timing to avoid catch-up
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

    case 2: // Fire columns — bidirectional diffusion + multi-octave drifting noise sparks
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
      return;
  }

  // Push the rendered frame to the LEDs
  FastLED.show();
}
