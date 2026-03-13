
// LedMatrix.cpp
#include "LedMatrix.h"
namespace LedMatrix {
  CRGB leds[NUM_LEDS];
  static uint32_t lastFrameMs = 0;
  static bool s_isOffLatched = false;

  // Folder 1
  static uint32_t partyTime = 0;   // removed CRGBPalette16 copy to save SRAM

  // Folder 4
  static uint32_t spookyTime = 0;

  // External breath support for folder 4 sync
  // 0xFF = not set / use internal breathing
  static uint8_t s_externalSpookyBreath = 0xFF;

  // Internal fixed-point smoothed pulse for spooky breathing fallback:
  // store 8.8 fixed point (upper 8 bits = integer 0..255, lower 8 bits = fraction)
  static uint16_t s_spookyPulseQ8_8 = 0;

  // Folder 2
  static uint8_t fireHeat[32] = {0};
  static uint8_t glowTrack[32] = {0};
  static uint8_t flickerTick = 0;
  static uint8_t flickerJitter[32] = {0};
  static uint32_t sparkNoiseTime = 0;
  static uint32_t sparkNoiseTime2 = 0;
  static int32_t sparkDriftX = 0;

  static inline uint16_t blockBaseIndex(uint16_t column) { return column * 8; }

  static inline uint8_t mixWeighted(uint8_t self, uint8_t neighbor, uint8_t neighborWeight) {
    uint16_t acc = (uint16_t)self * (255 - neighborWeight) + (uint16_t)neighbor * neighborWeight;
    return (uint8_t)(acc / 255);
  }
  static inline uint8_t mix8(uint8_t a, uint8_t b, uint8_t w) { return mixWeighted(a, b, w); }

  static inline uint16_t triwave16_local(uint16_t phase) {
    if (phase < 32768) {
      return (uint16_t)((uint32_t)phase * 2U);
    } else {
      return (uint16_t)((uint32_t)(65535U - phase) * 2U);
    }
  }

  void setSpookyBreath(uint8_t pulse) {
    s_externalSpookyBreath = pulse;
  }
} // namespace LedMatrix

using namespace LedMatrix;

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
void LedMatrix::begin() {
  FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  clear();
  FastLED.show();
  lastFrameMs = millis();
  s_isOffLatched = false;
}
void LedMatrix::clear() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

// ------------------------------------------------------------
// Renderers
// ------------------------------------------------------------
static void renderPartyMarble() {
  partyTime += LedMatrix::PARTY_TIME_NOISE_SPEED;
  const uint8_t swirlA = beatsin8(7, 0, 255);
  const uint8_t swirlB = beatsin8(13, 0, 255);

  for (uint16_t col = 0; col < LedMatrix::MATRIX_WIDTH; ++col) {
    uint32_t x = (uint32_t)col * LedMatrix::PARTY_COLUMN_NOISE_SCALE;
    uint16_t z = (uint16_t)(partyTime + (swirlA / 2) + (swirlB / 3));
    uint8_t n = inoise8((uint16_t)(x & 0xFFFF), z);
    uint8_t index = qadd8(n, 30);

    // Use PartyColors_p directly (no SRAM palette copy).
    CRGB color = ColorFromPalette(PartyColors_p, index, 255);

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = color;
  }
}

static void renderFireColumns() {
  for (uint8_t col = 0; col < 32; ++col) {
    uint8_t cooldown = random8(0, ((FIRE_COOLING * 10) / 32) + FIRE_FLICKER_VARIANCE);
    fireHeat[col] = (fireHeat[col] > cooldown) ? (fireHeat[col] - cooldown) : 0;
  }

  sparkNoiseTime += FIRE_SPARK_NOISE_SPEED;
  sparkNoiseTime2 += FIRE_SPARK_NOISE_SPEED2;
  sparkDriftX += (int32_t)FIRE_SPARK_DRIFT_SPEED;

  for (uint8_t col = 0; col < 32; ++col) {
    uint16_t x1 = (uint16_t)(col * FIRE_SPARK_NOISE_SCALE + (sparkDriftX & 0xFFFF));
    uint16_t x2 = (uint16_t)(col * FIRE_SPARK_NOISE_SCALE2 - ((sparkDriftX / 2) & 0xFFFF));
    uint8_t n1 = inoise8(x1, (uint16_t)sparkNoiseTime);
    uint8_t n2 = inoise8(x2, (uint16_t)sparkNoiseTime2);
    uint8_t nCombined = mix8(n1, n2, FIRE_SPARK_OCTAVE_BLEND);

    uint8_t prob = (uint8_t)map(nCombined, 0, 255, FIRE_SPARK_MIN_PROB, FIRE_SPARK_MAX_PROB);
    if (random8() < prob) {
      fireHeat[col] = qadd8(
        fireHeat[col],
        random8(FIRE_GLOBAL_SPARK_HEAT_MIN, FIRE_GLOBAL_SPARK_HEAT_MAX)
      );
    }
  }

  for (uint8_t col = 31; col > 0; --col) {
    fireHeat[col] = mixWeighted(fireHeat[col], fireHeat[col - 1], FIRE_DIFFUSE_WEIGHT);
  }
  for (uint8_t col = 0; col < 31; ++col) {
    fireHeat[col] = mixWeighted(fireHeat[col], fireHeat[col + 1], FIRE_DIFFUSE_WEIGHT);
  }

  for (uint8_t col = 0; col < 32; ++col) {
    glowTrack[col] = lerp8by8(glowTrack[col], fireHeat[col], GLOW_TRACK_WEIGHT);
  }

  flickerTick++;
  const bool updateJitterNow = (flickerTick % FIRE_FLICKER_PERIOD) == 0;
  if (updateJitterNow) {
    for (uint8_t col = 0; col < 32; ++col) {
      flickerJitter[col] = random8(0, FIRE_FLICKER_VARIANCE);
    }
  }

  for (uint8_t col = 0; col < 32; ++col) {
    uint8_t glowV = (uint8_t)map(glowTrack[col], 0, 255, GLOW_VAL_MIN, GLOW_VAL_MAX);
    CHSV glowHSV(GLOW_HUE, GLOW_SAT, glowV);
    CRGB baseGlow = glowHSV;

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = baseGlow;

    uint8_t jitter = flickerJitter[col];
    uint8_t vBase = qadd8(FIRE_BASE_BRIGHTNESS, jitter);
    CRGB flame = ColorFromPalette(HeatColors_p, fireHeat[col], vBase);

    for (uint8_t i = 0; i < 8; ++i) {
      CRGB &px = leds[base + i];
      px.r = max(px.r, flame.r);
      px.g = max(px.g, flame.g);
      px.b = max(px.b, flame.b);
    }
  }
}

static void renderXmasColumns() {
  uint8_t brightA = beatsin8(XMAS_PULSE_BPM, XMAS_MIN_BRIGHT, XMAS_MAX_BRIGHT, 0, 0);
  uint8_t brightB = beatsin8(XMAS_PULSE_BPM, XMAS_MIN_BRIGHT, XMAS_MAX_BRIGHT, 0, 128);

  for (uint16_t col = 0; col < 32; ++col) {
    CRGB color = (col < 16) ? CRGB(brightA, 0, 0) : CRGB(0, brightB, 0);
    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = color;
  }
}

static void renderSpookyColumns() {
  spookyTime += SPOOKY_TIME_NOISE_SPEED;

  uint8_t pulse = 0;
  if (s_externalSpookyBreath != 0xFF) {
    pulse = s_externalSpookyBreath;
  } else {
    const uint8_t SPOOKY_PULSE_BPM = 9;
    const uint8_t PULSE_MIN = 20;
    const uint8_t PULSE_MAX = 150;

    const uint16_t rangeQ8_8 = (uint16_t)(PULSE_MAX - PULSE_MIN) << 8;
    const uint16_t phase16 = beat16(SPOOKY_PULSE_BPM);
    const uint16_t tri16 = triwave16_local(phase16);
    const uint16_t targetQ8_8 =
      (uint16_t)(PULSE_MIN << 8) + (uint16_t)(((uint32_t)tri16 * (uint32_t)rangeQ8_8) >> 16);

    if (s_spookyPulseQ8_8 == 0) s_spookyPulseQ8_8 = targetQ8_8;
    int16_t diff = (int16_t)(targetQ8_8 - s_spookyPulseQ8_8);
    s_spookyPulseQ8_8 = (uint16_t)(s_spookyPulseQ8_8 + (diff >> 4));
    pulse = (uint8_t)(s_spookyPulseQ8_8 >> 8);
  }

  const uint8_t EDGE_W = 2;
  uint8_t edgeV = scale8(pulse, 208);
  if (edgeV < 45) edgeV = 45;
  const CRGB edgePurple = CHSV(205, 255, edgeV);

  const uint8_t SPOOKY_HUE_MIN = 60;
  const uint8_t SPOOKY_HUE_MAX = 124;

  for (uint16_t col = 0; col < 32; ++col) {
    const uint16_t x = (uint16_t)((uint32_t)col * SPOOKY_COLUMN_NOISE_SCALE);
    const uint16_t z = (uint16_t)spookyTime;

    uint8_t n1 = inoise8(x, z);
    uint8_t n2 = inoise8((uint16_t)(x >> 1), (uint16_t)(z + 137));
    uint8_t n = lerp8by8(n1, n2, 96);
    n = ease8InOutQuad(n);

    uint8_t hue = (uint8_t)map(n, 0, 255, SPOOKY_HUE_MIN, SPOOKY_HUE_MAX);
    uint8_t sat = (uint8_t)map(n2, 0, 255, 180, 255);
    CRGB color = CHSV(hue, sat, pulse);

    if (col < EDGE_W || col >= (32 - EDGE_W)) {
      color = edgePurple;
    }

    uint16_t base = blockBaseIndex(col);
    for (uint8_t i = 0; i < 8; ++i) leds[base + i] = color;
  }
}

// ------------------------------------------------------------
// Update dispatcher
// ------------------------------------------------------------
void LedMatrix::update(uint8_t folder, bool lightsOn) {
  const uint32_t now = millis();

  if (!lightsOn || folder == 99) {
    if (!s_isOffLatched) {
      clear();
      FastLED.show();
      s_isOffLatched = true;
    }
    lastFrameMs = now;
    return;
  }

  if (s_isOffLatched) s_isOffLatched = false;

  if (now - lastFrameMs < FRAME_INTERVAL_MS) return;
  lastFrameMs = now;

  switch (folder) {
    case 1: renderPartyMarble(); break;
    case 2:
      for (uint8_t i = 0; i < FIRE_SPEED_SCALE; ++i) renderFireColumns();
      break;
    case 3: renderXmasColumns(); break;
    case 4: renderSpookyColumns(); break;
    default:
      clear();
      FastLED.show();
      return;
  }

  FastLED.show();
}
