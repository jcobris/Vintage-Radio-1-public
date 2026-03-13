
// LedStrip.cpp
#include "LedStrip.h"
#include "Config.h"

// NOTE:
// This module intentionally does NOT call FastLED.show() inside update().
// The main loop should call LedStrip::update() BEFORE LedMatrix::update().
// LedMatrix::update() calls FastLED.show() which updates ALL controllers.

namespace LedStrip {

  // Hardware
  static const uint8_t  DATA_PIN     = Config::PIN_STRIP_DATA;
  static const uint16_t NUM_LEDS     = Config::STRIP_NUM_LEDS;
  static const EOrder   COLOR_ORDER  = GRB;

  // Frame pacing
  static const uint16_t FRAME_INTERVAL_MS = 30;

  // LED buffer (SRAM: NUM_LEDS * 3)
  static CRGB s_leds[NUM_LEDS];

  // State
  static uint32_t s_lastFrameMs   = 0;
  static bool     s_isOffLatched  = false;
  static uint8_t  s_lastFolder    = 255;

  // Folder 1 (Party)
  static uint32_t s_partyTime     = 0;

  // Folder 2 (Embers) - NO per-LED heat buffer (SRAM saver)
  static uint32_t s_emberTime     = 0;
  static int32_t  s_emberDriftX   = 0;

  // Folder 4 (Spooky) - external breath override (0xFF = not set)
  static uint8_t  s_externalSpookyBreath = 0xFF;
  static uint32_t s_spookyTime   = 0;

  void clear() {
    fill_solid(s_leds, NUM_LEDS, CRGB::Black);
  }

  void setSpookyBreath(uint8_t pulse) {
    s_externalSpookyBreath = pulse;
  }

  // ------------------------------------------------------------
  // Renderers
  // ------------------------------------------------------------

  // Folder 1: Party / rainbow marble-ish
  static inline void renderPartyMarbleStrip() {
    s_partyTime += 3;

    const uint16_t NOISE_SCALE = 380;
    const uint8_t  BRIGHT      = 255;

    const uint8_t swirlA = beatsin8(7, 0, 255);
    const uint8_t swirlB = beatsin8(13, 0, 255);

    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
      const uint16_t x = (uint16_t)(i * NOISE_SCALE);
      const uint16_t z = (uint16_t)(s_partyTime + (swirlA / 2) + (swirlB / 3));
      uint8_t n = inoise8(x, z);
      uint8_t index = qadd8(n, 30);
      s_leds[i] = ColorFromPalette(PartyColors_p, index, BRIGHT);
    }
  }

  // Folder 2: Ember glow (fireplace vibe) - NO heat buffer
  static inline void renderEmberGlowStrip() {
    // This deliberately avoids a per-LED heat array to save SRAM.
    // Noise is temporally coherent, so it still looks smooth, especially at 30ms frames.

    // Slow movement
    s_emberTime += 1;
    s_emberDriftX += 1;

    // Wide patches for 128 LED strip
    const uint16_t NOISE_SCALE = 220;

    // Ember range: keep it in red/orange (avoid white-hot flame)
    const uint8_t EMBER_INDEX_MIN = 18;   // deep red
    const uint8_t EMBER_INDEX_MAX = 140;  // orange

    // Brightness shaping
    const uint8_t VAL_MIN = 120;
    const uint8_t VAL_MAX = 255;

    // Rare small pops
    const uint8_t POP_PROB = 3;           // out of 255, per LED per frame (very rare)
    const uint8_t POP_ADD_R = 40;
    const uint8_t POP_ADD_G = 15;

    // Very light blur to widen glow without losing detail
    const uint8_t COLOR_BLUR_AMOUNT = 12;

    // Gentle global shimmer
    const uint8_t shimmer = beatsin8(6, 0, 25);

    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
      const uint16_t x = (uint16_t)((uint32_t)i * NOISE_SCALE + (uint16_t)(s_emberDriftX & 0xFFFF));
      const uint8_t  n = inoise8(x, (uint16_t)s_emberTime);

      // Index controls the HeatColors palette pick (red->orange->yellow->white)
      uint8_t idx = (uint8_t)map(n, 0, 255, EMBER_INDEX_MIN, EMBER_INDEX_MAX);

      // Brightness controls intensity (keep mostly bright but modulated)
      uint8_t val = (uint8_t)map(n, 0, 255, VAL_MIN, VAL_MAX);
      val = qadd8(val, shimmer);

      CRGB c = ColorFromPalette(HeatColors_p, idx, val, LINEARBLEND);

      // Subtle ember "pop"
      if (random8() < POP_PROB) {
        c.r = qadd8(c.r, POP_ADD_R);
        c.g = qadd8(c.g, POP_ADD_G);
      }

      s_leds[i] = c;
    }

    blur1d(s_leds, NUM_LEDS, COLOR_BLUR_AMOUNT);
  }

  // Folder 3: Christmas half pulse (left red, right green)
  static inline void renderXmasHalfPulseStrip() {
    const uint8_t MIN_BRIGHT = 30;
    const uint8_t MAX_BRIGHT = 255;
    const uint8_t BPM        = 18;

    const uint8_t brightA = beatsin8(BPM, MIN_BRIGHT, MAX_BRIGHT, 0, 0);
    const uint8_t brightB = beatsin8(BPM, MIN_BRIGHT, MAX_BRIGHT, 0, 128);

    const uint16_t half = (uint16_t)(NUM_LEDS / 2);
    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
      if (i < half) {
        s_leds[i] = CRGB(brightA, 0, 0);
      } else {
        s_leds[i] = CRGB(0, brightB, 0);
      }
    }
  }

  // Folder 4: Spooky green/aqua marbling, brightness driven by external shared breath
  static inline void renderSpookyStrip() {
    s_spookyTime += 1;

    uint8_t pulse = (s_externalSpookyBreath != 0xFF) ? s_externalSpookyBreath : 90;

    // Avoid FastLED macro collisions like HUE_MAX.
    const uint8_t SPOOKY_HUE_MIN = 70;
    const uint8_t SPOOKY_HUE_MAX = 120;

    const uint16_t NOISE_SCALE = 520;

    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
      const uint16_t x = (uint16_t)(i * NOISE_SCALE);
      const uint16_t z = (uint16_t)s_spookyTime;

      uint8_t n1 = inoise8(x, z);
      uint8_t n2 = inoise8((uint16_t)(x >> 1), (uint16_t)(z + 137));
      uint8_t n  = lerp8by8(n1, n2, 96);
      n = ease8InOutQuad(n);

      const uint8_t hue = (uint8_t)map(n, 0, 255, SPOOKY_HUE_MIN, SPOOKY_HUE_MAX);
      const uint8_t sat = (uint8_t)map(n2, 0, 255, 180, 255);

      s_leds[i] = CHSV(hue, sat, pulse);
    }
  }

  // ------------------------------------------------------------
  // Public API
  // ------------------------------------------------------------

  void begin() {
    FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(s_leds, NUM_LEDS);
    clear();

    // One-time show at boot so strip comes up in a known state.
    // (Runtime updates remain owned by LedMatrix::update().)
    FastLED.show();

    s_lastFrameMs  = millis();
    s_isOffLatched = false;
    s_lastFolder   = 255;

    s_partyTime    = 0;
    s_spookyTime   = 0;
    s_externalSpookyBreath = 0xFF;

    s_emberTime    = 0;
    s_emberDriftX  = 0;

    DBG_LED_STRIP2(F("[STRIP] Ready: "), NUM_LEDS);
  }

  void update(uint8_t folder, bool lightsOn) {
    // Off conditions: MATRIX_OFF (lightsOn=false) or folder==99
    if (!lightsOn || folder == 99) {
      if (!s_isOffLatched) {
        clear();
        s_isOffLatched = true;
        // No FastLED.show() here; LedMatrix::update() will push the cleared state.
      }
      s_lastFolder = folder;
      return;
    }

    // Coming back on
    if (s_isOffLatched) {
      s_isOffLatched = false;
      s_lastFrameMs = 0; // allow immediate render
    }

    // Gentle deterministic resets on folder change
    if (folder != s_lastFolder) {
      s_lastFolder = folder;

      if (folder == 1) {
        s_partyTime = 0;
      } else if (folder == 2) {
        s_emberTime = 0;
      } else if (folder == 4) {
        s_spookyTime = 0;
      }
    }

    const uint32_t now = millis();
    if ((uint32_t)(now - s_lastFrameMs) < FRAME_INTERVAL_MS) return;
    s_lastFrameMs = now;

    switch (folder) {
      case 1: renderPartyMarbleStrip(); break;
      case 2: renderEmberGlowStrip(); break; // ember glow, SRAM-friendly
      case 3: renderXmasHalfPulseStrip(); break;
      case 4: renderSpookyStrip(); break;
      default: clear(); break;
    }

    // IMPORTANT: Do not call FastLED.show() here.
    // LedMatrix::update() already calls FastLED.show() and will output both controllers.
  }

} // namespace LedStrip
