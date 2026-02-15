
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

  // Fire columns: per-column heat (0..255)
  // Two independent fronts (left & right), moving inward (non-mirrored randomness)
  static uint8_t       heatL[32] = {0};
  static uint8_t       heatR[32] = {0};

  // Wider flicker time base
  static uint32_t      fireFlickerTime = 0;

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

// Folder 2: Fire columns (two-front inward flames, red-biased, warm glow)
static void renderFireColumns() {
  // ===== Cooling both fronts =====
  for (uint8_t col = 0; col < 32; ++col) {
    uint8_t cooldownL = random8(0, ((LedMatrix::FIRE_COOLING * 10) / 32));
    uint8_t cooldownR = random8(0, ((LedMatrix::FIRE_COOLING * 10) / 32));
    heatL[col] = (heatL[col] > cooldownL) ? (heatL[col] - cooldownL) : 0;
    heatR[col] = (heatR[col] > cooldownR) ? (heatR[col] - cooldownR) : 0;
  }

  // ===== Sparks near ends (independent randomness) =====
  if (random8() < LedMatrix::FIRE_SPARKING) {
    uint8_t posL = random8(0, LedMatrix::FIRE_SPARK_ZONE_WIDTH); // left edge zone
    heatL[posL] = qadd8(heatL[posL], random8(LedMatrix::FIRE_SPARK_HEAT_MIN,
                                             LedMatrix::FIRE_SPARK_HEAT_MAX));
  }
  if (random8() < LedMatrix::FIRE_SPARKING) {
    uint8_t posR = 31 - random8(0, LedMatrix::FIRE_SPARK_ZONE_WIDTH); // right edge zone
    heatR[posR] = qadd8(heatR[posR], random8(LedMatrix::FIRE_SPARK_HEAT_MIN,
                                             LedMatrix::FIRE_SPARK_HEAT_MAX));
  }

  // ===== Inward diffusion/advection (left -> center, right -> center) =====
  // Left front moves right: smear from left neighbor into current (rightward)
  for (uint8_t col = 31; col > 0; --col) {
    heatL[col] = (heatL[col] + heatL[col - 1]) / 2;
  }
  // Right front moves left: smear from right neighbor into current (leftward)
  for (uint8_t col = 0; col < 31; ++col) {
    heatR[col] = (heatR[col] + heatR[col + 1]) / 2;
  }

  // ===== Final flicker time evolution =====
  fireFlickerTime += LedMatrix::FIRE_FLICKER_TIME_SPEED;

  // ===== Render columns with punchy combine (max of the two fronts) =====
  for (uint8_t col = 0; col < 32; ++col) {
    const uint8_t heat = max(heatL[col], heatR[col]); // punchy combine

    // Background glow mapped to visible range (slightly brighter but balanced)
    const uint8_t glowVal = (uint8_t)map(heat, 0, 255,
                                         LedMatrix::FIRE_GLOW_MIN_VAL,
                                         LedMatrix::FIRE_GLOW_MAX_VAL);
    const CHSV glowHSV(LedMatrix::FIRE_GLOW_HUE,
                       LedMatrix::FIRE_GLOW_SAT,
                       glowVal);
    const CRGB glow = glowHSV;

    // Wider flicker factor via 1D noise (spatial + temporal)
    const uint8_t flickerN = inoise8(
      (uint16_t)(col * LedMatrix::FIRE_FLICKER_SPATIAL_SCALE),
      (uint16_t)fireFlickerTime
    );
    const int8_t  flickerDelta = (int8_t)map(
      flickerN, 0, 255,
      -LedMatrix::FIRE_FLICKER_DEPTH,
       LedMatrix::FIRE_FLICKER_DEPTH
    );

    // Base flame brightness from heat (+ flicker variation)
    int16_t flameBright16 = map(heat, 0, 255,
                                LedMatrix::FIRE_FLAME_BRIGHT_MIN,
                                LedMatrix::FIRE_FLAME_BRIGHT_MAX);
    flameBright16 += flickerDelta;
    if (flameBright16 < 0)   flameBright16 = 0;
    if (flameBright16 > 255) flameBright16 = 255;
    uint8_t flameBright = (uint8_t)flameBright16;  // mutable

    // Palette index: cap to avoid constant yellow/white; flicker nudges hue a bit
    int16_t idx16 = heat;
    idx16 = min<int16_t>(idx16, LedMatrix::FIRE_PALETTE_CAP);
    idx16 += (flickerDelta / 2);

    // Red bias: shift palette index down toward reds/oranges
    idx16 -= LedMatrix::FIRE_RED_IDX_BIAS;

    if (idx16 < 0)   idx16 = 0;
    if (idx16 > 255) idx16 = 255;
    uint8_t idx = (uint8_t)idx16;

    // Flame color (HeatColors_p) + red/green channel bias
    CRGB flame = ColorFromPalette(HeatColors_p, idx, flameBright, LINEARBLEND);
    flame.r = qadd8(flame.r, LedMatrix::FIRE_RED_CHANNEL_BOOST);
    flame.g = qsub8(flame.g, LedMatrix::FIRE_GREEN_CHANNEL_REDUCE);

    // Nonlinear blend curve: glow at low heat, flame quickly dominates as heat rises
    const uint8_t eased   = ease8InOutApprox(heat); // 0..255 S-curve
    uint8_t blendAmt = (uint8_t)map(eased, 0, 255,
                                    LedMatrix::FIRE_BLEND_MIN_AMOUNT,
                                    LedMatrix::FIRE_BLEND_MAX_AMOUNT);

    const uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) {
      leds[base + i] = blend(glow, flame, blendAmt);
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
    uint16_t base = blockBaseIndex(col);
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
  if (now - lastFrameMs < LedMatrix::FRAME_INTERVAL_MS) return;
  lastFrameMs = now;

  // Dispatch by folder
  switch (folder) {
    case 1: // Party marble (smooth & relaxed)
      renderPartyMarble();
      break;

    case 2: // Fire columns (two-front inward flames, red-biased, warm glow)
      for (uint8_t i = 0; i < LedMatrix::FIRE_SPEED_SCALE; ++i) {
        renderFireColumns();
      }
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
