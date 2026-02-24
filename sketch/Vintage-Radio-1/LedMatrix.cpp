
// LedMatrix.cpp
#include "LedMatrix.h"

/*
  ============================================================
  Implementation notes
  ============================================================

  Column-color rule:
  - This matrix is treated as 32 "columns", each containing 8 LEDs.
  - Every renderer computes a single CRGB per column and applies it to the 8 LEDs.

  OFF / GAP behaviour:
  - If lightsOn==false OR folder==99:
      - clear()
      - FastLED.show()
      - latch into an "off" state so we do not spam show() every loop
  - When we return to a real folder with lightsOn==true, the latch is released.

  Frame throttling:
  - Rendering is limited to one frame per FRAME_INTERVAL_MS.
*/

namespace LedMatrix {

  // LED frame buffer
  CRGB leds[NUM_LEDS];

  // Frame timing
  static uint32_t lastFrameMs = 0;

  // OFF latch:
  // true  = matrix has already been cleared and shown for OFF/GAP state
  // false = normal rendering is active
  static bool s_isOffLatched = false;

  // ------------------------------------------------------------
  // Folder 1 (Party marble) state
  // ------------------------------------------------------------
  static CRGBPalette16 partyPalette = PartyColors_p;
  static uint32_t partyTime = 0;

  // ------------------------------------------------------------
  // Folder 4 (Spooky) palette + state
  // ------------------------------------------------------------
  static const TProgmemRGBPalette16 SpookyAquaGreen_p PROGMEM = {
    0x10C0FF, 0x20D8FF, 0x30F0FF, 0x20FFE0,
    0x10FFC0, 0x00FFA0, 0x00FF80, 0x20FF60,
    0x40FF40, 0x30E040, 0x20C050, 0x10A070,
    0x1080A0, 0x30A0C0, 0x20A8B0, 0x208880
  };
  static CRGBPalette16 spookyPalette = SpookyAquaGreen_p;
  static uint32_t spookyTime = 0;

  // ------------------------------------------------------------
  // Folder 2 (Fire) column state
  // ------------------------------------------------------------
  // One heat value per column (32 columns)
  static uint8_t fireHeat[32] = {0};

  // Glow base tracking (smoothed)
  static uint8_t glowTrack[32] = {0};

  // Flicker control
  static uint8_t flickerTick = 0;
  static uint8_t flickerJitter[32] = {0};

  // Noise time + horizontal drift for sparks
  static uint32_t sparkNoiseTime  = 0;
  static uint32_t sparkNoiseTime2 = 0;
  static int32_t  sparkDriftX     = 0;

  // ------------------------------------------------------------
  // Helpers
  // ------------------------------------------------------------

  // Each column is stored as a contiguous block of 8 LEDs in the buffer.
  static inline uint16_t blockBaseIndex(uint16_t column) { return column * 8; }

  // Weighted mixing helper for heat diffusion and noise blending
  static inline uint8_t mixWeighted(uint8_t self, uint8_t neighbor, uint8_t neighborWeight) {
    uint16_t acc = (uint16_t)self * (255 - neighborWeight) + (uint16_t)neighbor * neighborWeight;
    return (uint8_t)(acc / 255);
  }

  static inline uint8_t mix8(uint8_t a, uint8_t b, uint8_t w) { return mixWeighted(a, b, w); }

} // namespace LedMatrix

using namespace LedMatrix;

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
void LedMatrix::begin() {
  FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  clear();
  FastLED.show();
  lastFrameMs = millis();
  s_isOffLatched = false;
}

void LedMatrix::clear() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

// ------------------------------------------------------------
// Renderers (one color per column)
// ------------------------------------------------------------

static void renderPartyMarble() {
  partyTime += LedMatrix::PARTY_TIME_NOISE_SPEED;

  // Two slow oscillators create additional motion/variation
  const uint8_t swirlA = beatsin8(7, 0, 255);
  const uint8_t swirlB = beatsin8(13, 0, 255);

  for (uint16_t col = 0; col < LedMatrix::MATRIX_WIDTH; ++col) {
    uint32_t x = (uint32_t)col * LedMatrix::PARTY_COLUMN_NOISE_SCALE;
    uint16_t z = (uint16_t)(partyTime + (swirlA / 2) + (swirlB / 3));

    // Noise drives palette index
    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);
    uint8_t index = qadd8(n, 30);

    CRGB color = ColorFromPalette(LedMatrix::partyPalette, index, 255);

    // Apply same color to the whole 8-LED column
    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = color;
  }
}

static void renderFireColumns() {
  // Cool down each column a little
  for (uint8_t col = 0; col < 32; ++col) {
    uint8_t cooldown = random8(0, ((FIRE_COOLING * 10) / 32) + FIRE_FLICKER_VARIANCE);
    fireHeat[col] = (fireHeat[col] > cooldown) ? (fireHeat[col] - cooldown) : 0;
  }

  // Advance noise time and drift for sparks
  sparkNoiseTime  += FIRE_SPARK_NOISE_SPEED;
  sparkNoiseTime2 += FIRE_SPARK_NOISE_SPEED2;
  sparkDriftX     += (int32_t)FIRE_SPARK_DRIFT_SPEED;

  // Global sparks: noise-driven probability to add heat to columns
  for (uint8_t col = 0; col < 32; ++col) {
    uint16_t x1 = (uint16_t)(col * FIRE_SPARK_NOISE_SCALE  + (sparkDriftX & 0xFFFF));
    uint16_t x2 = (uint16_t)(col * FIRE_SPARK_NOISE_SCALE2 - ((sparkDriftX / 2) & 0xFFFF));

    uint8_t n1 = inoise8(x1, (uint16_t)sparkNoiseTime);
    uint8_t n2 = inoise8(x2, (uint16_t)sparkNoiseTime2);

    uint8_t nCombined = mix8(n1, n2, FIRE_SPARK_OCTAVE_BLEND);

    uint8_t prob = (uint8_t)map(nCombined, 0, 255, FIRE_SPARK_MIN_PROB, FIRE_SPARK_MAX_PROB);
    if (random8() < prob) {
      fireHeat[col] = qadd8(fireHeat[col],
                            random8(FIRE_GLOBAL_SPARK_HEAT_MIN, FIRE_GLOBAL_SPARK_HEAT_MAX));
    }
  }

  // Diffuse heat horizontally (two passes)
  for (uint8_t col = 31; col > 0; --col) {
    fireHeat[col] = mixWeighted(fireHeat[col], fireHeat[col - 1], FIRE_DIFFUSE_WEIGHT);
  }
  for (uint8_t col = 0; col < 31; ++col) {
    fireHeat[col] = mixWeighted(fireHeat[col], fireHeat[col + 1], FIRE_DIFFUSE_WEIGHT);
  }

  // Track glow base (smoothed)
  for (uint8_t col = 0; col < 32; ++col) {
    glowTrack[col] = lerp8by8(glowTrack[col], fireHeat[col], GLOW_TRACK_WEIGHT);
  }

  // Flicker jitter update at a slower cadence
  flickerTick++;
  const bool updateJitterNow = (flickerTick % FIRE_FLICKER_PERIOD) == 0;
  if (updateJitterNow) {
    for (uint8_t col = 0; col < 32; ++col) {
      flickerJitter[col] = random8(0, FIRE_FLICKER_VARIANCE);
    }
  }

  // Render each column (glow base + flame highlight)
  for (uint8_t col = 0; col < 32; ++col) {

    // Base glow layer
    uint8_t glowV = (uint8_t)map(glowTrack[col], 0, 255, GLOW_VAL_MIN, GLOW_VAL_MAX);
    CHSV glowHSV(GLOW_HUE, GLOW_SAT, glowV);
    CRGB baseGlow = glowHSV;

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = baseGlow;

    // Flame highlight layer
    uint8_t jitter = flickerJitter[col];
    uint8_t vBase = qadd8(FIRE_BASE_BRIGHTNESS, jitter);
    CRGB flame = ColorFromPalette(HeatColors_p, fireHeat[col], vBase);

    // Max-blend flame over glow
    for (uint8_t i = 0; i < 8; ++i) {
      CRGB &px = leds[base + i];
      px.r = max(px.r, flame.r);
      px.g = max(px.g, flame.g);
      px.b = max(px.b, flame.b);
    }
  }
}

static void renderXmasColumns() {
  // Two pulses out of phase
  uint8_t brightA = beatsin8(XMAS_PULSE_BPM, XMAS_MIN_BRIGHT, XMAS_MAX_BRIGHT, 0, 0);
  uint8_t brightB = beatsin8(XMAS_PULSE_BPM, XMAS_MIN_BRIGHT, XMAS_MAX_BRIGHT, 0, 128);

  for (uint16_t col = 0; col < 32; ++col) {
    // Left half red, right half green
    CRGB color = (col < 16) ? CRGB(brightA, 0, 0) : CRGB(0, brightB, 0);

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = color;
  }
}

static void renderSpookyColumns() {
  spookyTime += SPOOKY_TIME_NOISE_SPEED;

  // Breathing pulse dims/brightens the whole palette
  uint8_t pulse = beatsin8(SPOOKY_PULSE_BPM, 30, 255);

  for (uint16_t col = 0; col < 32; ++col) {
    uint32_t x = (uint32_t)col * SPOOKY_COLUMN_NOISE_SCALE;
    uint16_t z = (uint16_t)(spookyTime);

    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);
    uint8_t index = qadd8(n, 40);

    CRGB color = ColorFromPalette(spookyPalette, index, 255);
    color.nscale8_video(pulse);

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = color;
  }
}

// ------------------------------------------------------------
// Update dispatcher
// ------------------------------------------------------------
void LedMatrix::update(uint8_t folder, bool lightsOn) {
  const uint32_t now = millis();

  // OFF / GAP handling:
  // - If lights are off or folder==99, clear and show ONCE per transition.
  if (!lightsOn || folder == 99) {
    if (!s_isOffLatched) {
      clear();
      FastLED.show();
      s_isOffLatched = true;
    }
    lastFrameMs = now;
    return;
  }

  // If we were previously off, unlatch on first "on" update
  if (s_isOffLatched) {
    s_isOffLatched = false;
  }

  // Frame rate limiting
  if (now - lastFrameMs < FRAME_INTERVAL_MS) return;
  lastFrameMs = now;

  // Render by folder
  switch (folder) {
    case 1:
      renderPartyMarble();
      break;

    case 2:
      for (uint8_t i = 0; i < FIRE_SPEED_SCALE; ++i) renderFireColumns();
      break;

    case 3:
      renderXmasColumns();
      break;

    case 4:
      renderSpookyColumns();
      break;

    default:
      // Safety: unknown folder -> blank
      clear();
      FastLED.show();
      return;
  }

  FastLED.show();
}
