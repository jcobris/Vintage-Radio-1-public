
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
  // Match the matrix cadence (you set matrix to ~30ms). This reduces strip buffer churn
  // and keeps strip animation cadence aligned with matrix updates.
  static const uint16_t FRAME_INTERVAL_MS = 30;

  // LED buffer (SRAM: 64 * 3 = 192 bytes)
  static CRGB s_leds[NUM_LEDS];

  // State
  static uint32_t s_lastFrameMs = 0;
  static bool     s_isOffLatched = false;
  static uint8_t  s_baseHue = 0;
  static uint8_t  s_lastFolder = 255;

  void clear() {
    fill_solid(s_leds, NUM_LEDS, CRGB::Black);
  }

  static inline void renderTestPattern(uint8_t folder) {
    // Simple rainbow test pattern with a per-folder hue offset
    const uint8_t folderOffset = (uint8_t)(folder * 16);
    const uint8_t startHue = (uint8_t)(s_baseHue + folderOffset);
    fill_rainbow(s_leds, NUM_LEDS, startHue, 5);
    s_baseHue++;
  }

  void begin() {
    FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(s_leds, NUM_LEDS);
    clear();

    // One-time show at boot so strip comes up in a known state.
    FastLED.show();

    s_lastFrameMs = millis();
    s_isOffLatched = false;
    s_baseHue = 0;
    s_lastFolder = 255;

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
      // allow immediate render on next frame
      s_lastFrameMs = 0;
    }

    // Optional: reset animation gently on folder change (keeps behaviour deterministic)
    if (folder != s_lastFolder) {
      s_lastFolder = folder;
      s_baseHue = (uint8_t)(folder * 32);
    }

    const uint32_t now = millis();
    if ((uint32_t)(now - s_lastFrameMs) < FRAME_INTERVAL_MS) return;
    s_lastFrameMs = now;

    // For now, render the test pattern (future: switch(folder) -> theme renderers)
    renderTestPattern(folder);

    // IMPORTANT: Do not call FastLED.show() here.
    // LedMatrix::update() already calls FastLED.show() and will output both controllers.
  }

} // namespace LedStrip
