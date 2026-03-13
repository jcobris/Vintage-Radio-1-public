
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

  // Folder 2 (Embers) - no per-LED heat buffer (SRAM saver)
  static uint32_t s_emberTime     = 0;
  static int32_t  s_emberDriftX   = 0;

  // Folder 4 (Spooky) - external breath override (0xFF = not set)
  static uint8_t  s_externalSpookyBreath = 0xFF;
  static uint32_t s_spookyTime   = 0;

  // ------------------------------------------------------------
  // Band widening control
  // ------------------------------------------------------------
  // Requested: widen folder 1,2,4 by factor of 4 => one computed colour spans 4 LEDs.
  static const uint8_t BLOCK_SIZE = 4;

  // Feathering: apply a light in-place blur after block rendering to reduce "hard steps".
  // These values are intentionally small so the 4-LED banding remains obvious but softened.
  static const uint8_t FEATHER_PARTY  = 18;
  static const uint8_t FEATHER_EMBER  = 14;
  static const uint8_t FEATHER_SPOOKY = 18;

  void clear() {
    fill_solid(s_leds, NUM_LEDS, CRGB::Black);
  }

  void setSpookyBreath(uint8_t pulse) {
    s_externalSpookyBreath = pulse;
  }

  // Write a single colour across a block [i .. i+BLOCK_SIZE-1], clamped to NUM_LEDS.
  static inline void fillBlock(uint16_t i, const CRGB &c) {
    for (uint8_t k = 0; k < BLOCK_SIZE; ++k) {
      const uint16_t idx = (uint16_t)(i + k);
      if (idx >= NUM_LEDS) break;
      s_leds[idx] = c;
    }
  }

  // Feather block edges without extra SRAM: a light blur is enough to soften the steps.
  static inline void featherEdges(uint8_t amount) {
    // blur1d works in-place on the existing LED buffer.
    // amount: 0..255 (higher = more blur).
    if (amount == 0) return;
    blur1d(s_leds, NUM_LEDS, amount);
  }

  // ------------------------------------------------------------
  // Renderers
  // ------------------------------------------------------------

  // Folder 1: Party / rainbow marble-ish (block-rendered x4 + feather)
  static inline void renderPartyMarbleStrip_Block4() {
    s_partyTime += 3;

    // Virtual pixels = blocks
    const uint16_t virtualCount = (uint16_t)((NUM_LEDS + (BLOCK_SIZE - 1)) / BLOCK_SIZE);

    // Keep existing "feel" but with wider bands due to virtualCount reduction.
    const uint16_t NOISE_SCALE = 380;
    const uint8_t  BRIGHT      = 255;

    const uint8_t swirlA = beatsin8(7, 0, 255);
    const uint8_t swirlB = beatsin8(13, 0, 255);

    for (uint16_t v = 0; v < virtualCount; ++v) {
      const uint16_t x = (uint16_t)(v * NOISE_SCALE);
      const uint16_t z = (uint16_t)(s_partyTime + (swirlA / 2) + (swirlB / 3));
      const uint8_t  n = inoise8(x, z);
      const uint8_t  index = qadd8(n, 30);

      const CRGB c = ColorFromPalette(PartyColors_p, index, BRIGHT);

      const uint16_t i = (uint16_t)(v * BLOCK_SIZE);
      fillBlock(i, c);
    }

    // Soften hard steps between blocks.
    featherEdges(FEATHER_PARTY);
  }

  // Folder 2: Ember glow (fireplace vibe) (block-rendered x4 + feather)
  static inline void renderEmberGlowStrip_Block4() {
    s_emberTime += 1;
    s_emberDriftX += 1;

    const uint16_t virtualCount = (uint16_t)((NUM_LEDS + (BLOCK_SIZE - 1)) / BLOCK_SIZE);

    const uint16_t NOISE_SCALE = 220;

    const uint8_t EMBER_INDEX_MIN = 18;
    const uint8_t EMBER_INDEX_MAX = 140;

    const uint8_t VAL_MIN = 120;
    const uint8_t VAL_MAX = 255;

    const uint8_t POP_PROB = 3;   // /255 per block per frame (very rare)
    const uint8_t POP_ADD_R = 40;
    const uint8_t POP_ADD_G = 15;

    const uint8_t shimmer = beatsin8(6, 0, 25);

    for (uint16_t v = 0; v < virtualCount; ++v) {
      const uint16_t x = (uint16_t)((uint32_t)v * NOISE_SCALE + (uint16_t)(s_emberDriftX & 0xFFFF));
      const uint8_t  n = inoise8(x, (uint16_t)s_emberTime);

      uint8_t idx = (uint8_t)map(n, 0, 255, EMBER_INDEX_MIN, EMBER_INDEX_MAX);
      uint8_t val = (uint8_t)map(n, 0, 255, VAL_MIN, VAL_MAX);
      val = qadd8(val, shimmer);

      CRGB c = ColorFromPalette(HeatColors_p, idx, val, LINEARBLEND);

      if (random8() < POP_PROB) {
        c.r = qadd8(c.r, POP_ADD_R);
        c.g = qadd8(c.g, POP_ADD_G);
      }

      const uint16_t i = (uint16_t)(v * BLOCK_SIZE);
      fillBlock(i, c);
    }

    // Slightly less feather than party/spooky so ember "chunks" remain readable.
    featherEdges(FEATHER_EMBER);
  }

  // Folder 3: Christmas half pulse (left red, right green) - UNCHANGED
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

  // Folder 4: Spooky green/aqua marbling, brightness driven by external shared breath (block-rendered x4 + feather)
  static inline void renderSpookyStrip_Block4() {
    s_spookyTime += 1;

    const uint16_t virtualCount = (uint16_t)((NUM_LEDS + (BLOCK_SIZE - 1)) / BLOCK_SIZE);

    // External shared breath MUST drive brightness (sync requirement).
    const uint8_t pulse = (s_externalSpookyBreath != 0xFF) ? s_externalSpookyBreath : 90;

    const uint8_t SPOOKY_HUE_MIN = 70;
    const uint8_t SPOOKY_HUE_MAX = 120;

    const uint16_t NOISE_SCALE = 520;

    for (uint16_t v = 0; v < virtualCount; ++v) {
      const uint16_t x = (uint16_t)(v * NOISE_SCALE);
      const uint16_t z = (uint16_t)s_spookyTime;

      uint8_t n1 = inoise8(x, z);
      uint8_t n2 = inoise8((uint16_t)(x >> 1), (uint16_t)(z + 137));
      uint8_t n  = lerp8by8(n1, n2, 96);
      n = ease8InOutQuad(n);

      const uint8_t hue = (uint8_t)map(n, 0, 255, SPOOKY_HUE_MIN, SPOOKY_HUE_MAX);
      const uint8_t sat = (uint8_t)map(n2, 0, 255, 180, 255);

      const CRGB c = CHSV(hue, sat, pulse);

      const uint16_t i = (uint16_t)(v * BLOCK_SIZE);
      fillBlock(i, c);
    }

    // Soften hard steps between blocks while keeping banding.
    featherEdges(FEATHER_SPOOKY);
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
    s_emberTime    = 0;
    s_emberDriftX  = 0;

    s_spookyTime   = 0;
    s_externalSpookyBreath = 0xFF;

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
      case 1: renderPartyMarbleStrip_Block4(); break; // widened x4 + feathered
      case 2: renderEmberGlowStrip_Block4();   break; // widened x4 + feathered
      case 3: renderXmasHalfPulseStrip();      break; // unchanged
      case 4: renderSpookyStrip_Block4();      break; // widened x4 + feathered (breath sync preserved)
      default: clear(); break;
    }

    // IMPORTANT: Do not call FastLED.show() here.
    // LedMatrix::update() already calls FastLED.show() and will output both controllers.
  }

} // namespace LedStrip
