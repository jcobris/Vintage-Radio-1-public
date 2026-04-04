
// LedStrip.cpp
#include "LedStrip.h"
#include "Config.h"

// NOTE:
// This module intentionally does NOT call FastLED.show() inside update().
// The main loop should call LedStrip::update() BEFORE LedMatrix::update().
// LedMatrix::update() calls FastLED.show() which updates ALL controllers.

namespace LedStrip {

  // Hardware
  static const uint8_t  DATA_PIN    = Config::PIN_STRIP_DATA;
  static const uint16_t NUM_LEDS    = Config::STRIP_NUM_LEDS;
  static const EOrder   COLOR_ORDER = GRB;

  // Frame pacing
  static const uint16_t FRAME_INTERVAL_MS = 30;

  // LED buffer (SRAM: NUM_LEDS * 3)
  static CRGB s_leds[NUM_LEDS];

  // State
  static uint32_t s_lastFrameMs = 0;
  static bool     s_isOffLatched = false;
  static uint8_t  s_lastFolder = 255;

  // Folder 1 (Party)
  static uint32_t s_partyTime = 0;

  // Folder 2 (Embers) - marble/noise driven, no per-LED heat buffer (SRAM saver)
  static uint32_t s_emberTime = 0;
  static int32_t  s_emberDriftX = 0;

  // Folder 4 (Spooky) - external breath override (0xFF = not set)
  static uint8_t  s_externalSpookyBreath = 0xFF;
  static uint32_t s_spookyTime = 0;

  // ------------------------------------------------------------
  // Band widening control
  // ------------------------------------------------------------
  // Widen folder 1,2,4 by factor of 4 => one computed colour spans 4 LEDs.
  static const uint8_t BLOCK_SIZE = 4;

  // Feathering: light in-place blur after block rendering to reduce hard steps.
  static const uint8_t FEATHER_PARTY  = 18;
  // Increased for Folder 2 to reduce "stripy" look on a long 128 LED strip.
  static const uint8_t FEATHER_EMBER  = 34;
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

  static inline void featherEdges(uint8_t amount) {
    if (amount == 0) return;
    // blur1d operates in-place on s_leds[] (no extra SRAM buffers).
    blur1d(s_leds, NUM_LEDS, amount);
  }

  // ------------------------------------------------------------
  // Renderers
  // ------------------------------------------------------------

  // Folder 1: Party / rainbow marble-ish (block-rendered x4 + feather)
  static inline void renderPartyMarbleStrip_Block4() {
    s_partyTime += 3;

    const uint16_t virtualCount = (uint16_t)((NUM_LEDS + (BLOCK_SIZE - 1)) / BLOCK_SIZE);
    const uint16_t NOISE_SCALE = 380;
    const uint8_t  BRIGHT = 255;

    const uint8_t swirlA = beatsin8(7, 0, 255);
    const uint8_t swirlB = beatsin8(13, 0, 255);

    for (uint16_t v = 0; v < virtualCount; ++v) {
      const uint16_t x = (uint16_t)(v * NOISE_SCALE);
      const uint16_t z = (uint16_t)(s_partyTime + (swirlA / 2) + (swirlB / 3));
      const uint8_t  n = inoise8(x, z);
      const uint8_t  index = qadd8(n, 30);
      const CRGB     c = ColorFromPalette(PartyColors_p, index, BRIGHT);

      const uint16_t i = (uint16_t)(v * BLOCK_SIZE);
      fillBlock(i, c);
    }

    featherEdges(FEATHER_PARTY);
  }

  // Folder 2: Slow ember-bed marble (A1-style), darker lows, more blur, no white
  static inline void renderEmberMarbleSlow_Block4() {
    // Slow burn: reduce motion speed.
    s_emberTime += 1;
    s_emberDriftX += 1;

    const uint16_t virtualCount = (uint16_t)((NUM_LEDS + (BLOCK_SIZE - 1)) / BLOCK_SIZE);

    // Two-octave noise to reduce "stripy" artifacts in 1D:
    // A large-scale field + a smaller-scale field blended together.
    const uint16_t SCALE_COARSE = 360;
    const uint16_t SCALE_FINE = 120;
    const uint8_t BLEND_FINE = 80; // 0..255 (higher = more fine detail)

    // Brightness noise uses a separate scale/time offset so brightness pockets
    // don't perfectly align with color pockets (more organic coal bed).
    const uint16_t SCALE_BRIGHT = 220;

    // Palette index clamp to stay red/orange only (no white-hot end of HeatColors_p).
    const uint8_t IDX_MIN = 6;   // deep red
    const uint8_t IDX_MAX = 110; // orange (kept well below white/yellow-white region)

    // Darker lows for high contrast ("can turn quite low sometimes").
    const uint8_t VAL_MIN = 4;   // very dim lows
    const uint8_t VAL_MAX = 210; // strong glow but not pushing into white due to idx clamp

    // Calm shimmer (slow and subtle).
    const uint8_t shimmer = beatsin8(4, 0, 18);

    // Rare, gentle coal pops (A1-ish calm): fewer than A2, but still present.
    const uint8_t POP_PROB = 3; // /255 per block per frame (rare)
    const uint8_t POP_ADD_VAL_MIN = 10;
    const uint8_t POP_ADD_VAL_MAX = 45;
    const uint8_t POP_WARM_R = 22;
    const uint8_t POP_WARM_G = 8;

    for (uint16_t v = 0; v < virtualCount; ++v) {
      // Color field (two octaves blended)
      const uint16_t xC1 = (uint16_t)((uint32_t)v * SCALE_COARSE + (uint16_t)(s_emberDriftX & 0xFFFF));
      const uint16_t xC2 = (uint16_t)((uint32_t)v * SCALE_FINE + (uint16_t)((s_emberDriftX * 3) & 0xFFFF));
      const uint8_t nCoarse = inoise8(xC1, (uint16_t)(s_emberTime));
      const uint8_t nFine = inoise8(xC2, (uint16_t)(s_emberTime + 53));
      uint8_t nColor = lerp8by8(nCoarse, nFine, BLEND_FINE);
      nColor = ease8InOutQuad(nColor);

      // Brightness field (separately sampled)
      const uint16_t xB = (uint16_t)((uint32_t)v * SCALE_BRIGHT + (uint16_t)((s_emberDriftX * 5) & 0xFFFF));
      uint8_t nBright = inoise8(xB, (uint16_t)(s_emberTime + 97));
      nBright = ease8InOutQuad(nBright);

      // Map into ember-only palette region (no white).
      const uint8_t idx = (uint8_t)map(nColor, 0, 255, IDX_MIN, IDX_MAX);

      // Map brightness with very low floor (dark pockets), plus subtle shimmer.
      uint8_t val = (uint8_t)map(nBright, 0, 255, VAL_MIN, VAL_MAX);
      val = qadd8(val, shimmer);

      CRGB c = ColorFromPalette(HeatColors_p, idx, val, LINEARBLEND);

      // Rare coal pop: brighten + warm bias (still no white because idx is clamped).
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

    // More blur to reduce stripiness and soften band edges (still in-place).
    featherEdges(FEATHER_EMBER);
  }

  // Folder 3: Christmas half pulse (UPDATED: swapped colours + split offset -2)
  static inline void renderXmasHalfPulseStrip() {
    const uint8_t MIN_BRIGHT = 30;
    const uint8_t MAX_BRIGHT = 255;
    const uint8_t BPM = 18;

    const uint8_t brightA = beatsin8(BPM, MIN_BRIGHT, MAX_BRIGHT, 0, 0);
    const uint8_t brightB = beatsin8(BPM, MIN_BRIGHT, MAX_BRIGHT, 0, 128);

    const uint16_t half  = (uint16_t)(NUM_LEDS / 2);
    // Item 3: move the boundary left by 2 LEDs (minus 2 offset)
    const uint16_t split = (half + 2 <= NUM_LEDS) ? (uint16_t)(half + 2) : half;

    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
      if (i < split) {
        // LEFT side: GREEN
        s_leds[i] = CRGB(0, brightA, 0);
      } else {
        // RIGHT side: RED
        s_leds[i] = CRGB(brightB, 0, 0);
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
      uint8_t n = lerp8by8(n1, n2, 96);
      n = ease8InOutQuad(n);

      const uint8_t hue = (uint8_t)map(n, 0, 255, SPOOKY_HUE_MIN, SPOOKY_HUE_MAX);
      const uint8_t sat = (uint8_t)map(n2, 0, 255, 180, 255);

      const CRGB c = CHSV(hue, sat, pulse);

      const uint16_t i = (uint16_t)(v * BLOCK_SIZE);
      fillBlock(i, c);
    }

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

    s_lastFrameMs = millis();
    s_isOffLatched = false;
    s_lastFolder = 255;

    s_partyTime = 0;
    s_emberTime = 0;
    s_emberDriftX = 0;
    s_spookyTime = 0;
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
      case 2: renderEmberMarbleSlow_Block4(); break;  // slow, darker, blurrier ember coals
      case 3: renderXmasHalfPulseStrip(); break;       // UPDATED: swap + split offset -2
      case 4: renderSpookyStrip_Block4(); break;       // widened x4 + feathered (breath sync preserved)
      default: clear(); break;
    }

    // IMPORTANT: Do not call FastLED.show() here.
    // LedMatrix::update() already calls FastLED.show() and will output both controllers.
  }

} // namespace LedStrip
