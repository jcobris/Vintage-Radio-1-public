
// LedStrip.cpp
#include "LedStrip.h"
#include "Config.h"

// NOTE:
// This module intentionally does NOT call FastLED.show() inside update().
// The main loop should call LedStrip::update() BEFORE LedMatrix::update().
// LedMatrix::update() calls FastLED.show() which updates ALL controllers.

namespace LedStrip {

  static const uint8_t DATA_PIN = Config::PIN_STRIP_DATA;
  static const uint16_t NUM_LEDS = Config::STRIP_NUM_LEDS;
  static const EOrder COLOR_ORDER = GRB;

  static const uint16_t FRAME_INTERVAL_MS = 30;

  static CRGB s_leds[NUM_LEDS];

  static uint32_t s_lastFrameMs = 0;
  static bool s_isOffLatched = false;
  static uint8_t s_lastFolder = 255;

  static uint32_t s_partyTime = 0;

  static uint32_t s_emberTime = 0;
  static int32_t s_emberDriftX = 0;

  static uint8_t s_externalSpookyBreath = 0xFF;

  static const uint8_t BLOCK_SIZE = 4;

  static const uint8_t FEATHER_PARTY  = 18;
  static const uint8_t FEATHER_EMBER  = 34;
  static const uint8_t FEATHER_SPOOKY = 18;

  void clear() {
    fill_solid(s_leds, NUM_LEDS, CRGB::Black);
  }

  void setSpookyBreath(uint8_t pulse) {
    s_externalSpookyBreath = pulse;
  }

  static inline void fillBlock(uint16_t i, const CRGB &c) {
    for (uint8_t k = 0; k < BLOCK_SIZE; ++k) {
      const uint16_t idx = (uint16_t)(i + k);
      if (idx >= NUM_LEDS) break;
      s_leds[idx] = c;
    }
  }

  static inline void featherEdges(uint8_t amount) {
    if (amount == 0) return;
    blur1d(s_leds, NUM_LEDS, amount);
  }

  static inline void renderPartyMarbleStrip_Block4() {
    s_partyTime += 3;
    const uint16_t virtualCount = (uint16_t)((NUM_LEDS + (BLOCK_SIZE - 1)) / BLOCK_SIZE);
    const uint16_t NOISE_SCALE = 380;
    const uint8_t BRIGHT = 255;

    const uint8_t swirlA = beatsin8(7, 0, 255);
    const uint8_t swirlB = beatsin8(13, 0, 255);

    for (uint16_t v = 0; v < virtualCount; ++v) {
      const uint16_t x = (uint16_t)(v * NOISE_SCALE);
      const uint16_t z = (uint16_t)(s_partyTime + (swirlA / 2) + (swirlB / 3));
      const uint8_t n = inoise8(x, z);
      const uint8_t index = qadd8(n, 30);
      const CRGB c = ColorFromPalette(PartyColors_p, index, BRIGHT);
      const uint16_t i = (uint16_t)(v * BLOCK_SIZE);
      fillBlock(i, c);
    }

    featherEdges(FEATHER_PARTY);
  }

  static inline void renderEmberMarbleSlow_Block4() {
    s_emberTime += 1;
    s_emberDriftX += 1;

    const uint16_t virtualCount = (uint16_t)((NUM_LEDS + (BLOCK_SIZE - 1)) / BLOCK_SIZE);

    const uint16_t SCALE_COARSE = 360;
    const uint16_t SCALE_FINE = 120;
    const uint8_t BLEND_FINE = 80;

    const uint16_t SCALE_BRIGHT = 220;

    const uint8_t IDX_MIN = 6;
    const uint8_t IDX_MAX = 110;

    const uint8_t VAL_MIN = 4;
    const uint8_t VAL_MAX = 210;

    const uint8_t shimmer = beatsin8(4, 0, 18);

    const uint8_t POP_PROB = 3;
    const uint8_t POP_ADD_VAL_MIN = 10;
    const uint8_t POP_ADD_VAL_MAX = 45;
    const uint8_t POP_WARM_R = 22;
    const uint8_t POP_WARM_G = 8;

    for (uint16_t v = 0; v < virtualCount; ++v) {
      const uint16_t xC1 = (uint16_t)((uint32_t)v * SCALE_COARSE + (uint16_t)(s_emberDriftX & 0xFFFF));
      const uint16_t xC2 = (uint16_t)((uint32_t)v * SCALE_FINE + (uint16_t)((s_emberDriftX * 3) & 0xFFFF));
      const uint8_t nCoarse = inoise8(xC1, (uint16_t)(s_emberTime));
      const uint8_t nFine = inoise8(xC2, (uint16_t)(s_emberTime + 53));
      uint8_t nColor = lerp8by8(nCoarse, nFine, BLEND_FINE);
      nColor = ease8InOutQuad(nColor);

      const uint16_t xB = (uint16_t)((uint32_t)v * SCALE_BRIGHT + (uint16_t)((s_emberDriftX * 5) & 0xFFFF));
      uint8_t nBright = inoise8(xB, (uint16_t)(s_emberTime + 97));
      nBright = ease8InOutQuad(nBright);

      const uint8_t idx = (uint8_t)map(nColor, 0, 255, IDX_MIN, IDX_MAX);

      uint8_t val = (uint8_t)map(nBright, 0, 255, VAL_MIN, VAL_MAX);
      val = qadd8(val, shimmer);

      CRGB c = ColorFromPalette(HeatColors_p, idx, val, LINEARBLEND);

      if (random8() < POP_PROB) {
        const uint8_t addV = random8(POP_ADD_VAL_MIN, POP_ADD_VAL_MAX);
        const uint8_t newV = qadd8(val, addV);
        c = ColorFromPalette(HeatColors_p, idx, newV, LINEARBLEND);
        c.r = qadd8(c.r, POP_WARM_R);
        c.g = qadd8(c.g, POP_WARM_G);
      }

      const uint16_t i = (uint16_t)(v * BLOCK_SIZE);
      fillBlock(i, c);
    }

    featherEdges(FEATHER_EMBER);
  }

  static inline void renderXmasHalfPulseStrip() {
    const uint8_t MIN_BRIGHT = 30;
    const uint8_t MAX_BRIGHT = 255;
    const uint8_t BPM = 18;

    const uint8_t brightA = beatsin8(BPM, MIN_BRIGHT, MAX_BRIGHT, 0, 0);
    const uint8_t brightB = beatsin8(BPM, MIN_BRIGHT, MAX_BRIGHT, 0, 128);

    const uint16_t half = (uint16_t)(NUM_LEDS / 2);
    const uint16_t split = (half + 2 <= NUM_LEDS) ? (uint16_t)(half + 2) : half;

    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
      if (i < split) s_leds[i] = CRGB(0, brightA, 0);
      else           s_leds[i] = CRGB(brightB, 0, 0);
    }
  }

  static inline void renderSpookySolidStrip() {
    const uint8_t pulse = (s_externalSpookyBreath != 0xFF) ? s_externalSpookyBreath : 90;

    // More lime (match matrix)
    const CHSV fogHSV(100, 220, 255);
    CRGB fog = fogHSV;
    fog.nscale8_video(pulse);

    fill_solid(s_leds, NUM_LEDS, fog);
    featherEdges(FEATHER_SPOOKY);
  }

  void begin() {
    FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(s_leds, NUM_LEDS);
    clear();
    FastLED.show();

    s_lastFrameMs = millis();
    s_isOffLatched = false;
    s_lastFolder = 255;

    s_partyTime = 0;
    s_emberTime = 0;
    s_emberDriftX = 0;

    s_externalSpookyBreath = 0xFF;

    DBG_LED_STRIP2(F("[STRIP] Ready: "), NUM_LEDS);
  }

  void update(uint8_t folder, bool lightsOn) {
    if (!lightsOn || folder == 99) {
      if (!s_isOffLatched) {
        clear();
        s_isOffLatched = true;
      }
      s_lastFolder = folder;
      return;
    }

    if (s_isOffLatched) {
      s_isOffLatched = false;
      s_lastFrameMs = 0;
    }

    if (folder != s_lastFolder) {
      s_lastFolder = folder;
      if (folder == 1) s_partyTime = 0;
      else if (folder == 2) s_emberTime = 0;
    }

    const uint32_t now = millis();
    if ((uint32_t)(now - s_lastFrameMs) < FRAME_INTERVAL_MS) return;
    s_lastFrameMs = now;

    switch (folder) {
      case 1: renderPartyMarbleStrip_Block4(); break;
      case 2: renderEmberMarbleSlow_Block4(); break;
      case 3: renderXmasHalfPulseStrip(); break;
      case 4: renderSpookySolidStrip(); break;
      default: clear(); break;
    }

    // IMPORTANT: Do not call FastLED.show() here.
  }

} // namespace LedStrip
