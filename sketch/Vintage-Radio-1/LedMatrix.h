
// LedMatrix.h
#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <FastLED.h>
#include "Config.h"

/*
  ============================================================
  WS2812B 8x32 LED Matrix (Column-Color Constraint)
  ============================================================

  Hardware:
  - WS2812B matrix, wired as 8 rows x 32 columns (total 256 LEDs)
  - Data pin is defined in Config.h and referenced here via Config::PIN_MATRIX_DATA

  Key constraint (design requirement):
  - One solid color per column (8 LEDs per column are identical).
    This is enforced by rendering each column as a single CRGB and applying it
    to the 8 LEDs that make up that column.

  Operating modes:
  - Folder 1: Party / rainbow marble
  - Folder 2: Fire (one heat value per column, plus glow base)
  - Folder 3: Christmas (left half red pulse, right half green pulse)
  - Folder 4: Spooky (green/aqua palette with breathing pulse)

  Update model:
  - LedMatrix::update(folder, lightsOn) is non-blocking and frame-throttled.
  - If lightsOn is false OR folder==99, the matrix is cleared and latched OFF
    (cleared/shown once per transition) to avoid unnecessary FastLED.show() calls.

  Performance:
  - Uses a simple frame interval (FRAME_INTERVAL_MS).
  - Avoids per-frame expensive operations where possible.
*/

namespace LedMatrix {

  // ============================================================
  // Hardware & global settings
  // ============================================================

  // WS2812B DIN pin (single source of truth from Config.h)
  static const uint8_t DATA_PIN = Config::PIN_MATRIX_DATA;

  // Matrix geometry
  static const uint16_t MATRIX_WIDTH  = 32; // columns
  static const uint16_t MATRIX_HEIGHT = 8;  // rows
  static const uint16_t NUM_LEDS      = MATRIX_WIDTH * MATRIX_HEIGHT; // 256

  // WS2812 color order and global brightness
  static const EOrder COLOR_ORDER = GRB;
  static const uint8_t BRIGHTNESS = 50;

  // Frame pacing for non-blocking update
  static const uint16_t FRAME_INTERVAL_MS = 15;

  // ============================================================
  // Folder-specific parameters
  // ============================================================

  // Folder 1: Party marble (relaxed)
  static const uint16_t PARTY_COLUMN_NOISE_SCALE = 4000;
  static const uint16_t PARTY_TIME_NOISE_SPEED   = 3;

  // Folder 2: Fire columns
  // One heat value per column; color derived from HeatColors palette.
  static const uint8_t  FIRE_COOLING         = 55;
  static const uint8_t  FIRE_BASE_BRIGHTNESS = 255;

  // Fire flicker (calmer)
  static const uint8_t  FIRE_FLICKER_VARIANCE = 8;   // 0 disables jitter
  static const uint8_t  FIRE_FLICKER_PERIOD   = 16;  // update jitter every N frames
  static const uint8_t  FIRE_SPEED_SCALE      = 1;   // render steps per update

  // Fire global sparks (multi-octave noise with horizontal drift)
  static const uint16_t FIRE_SPARK_NOISE_SCALE  = 1800;
  static const uint16_t FIRE_SPARK_NOISE_SPEED  = 9;
  static const uint16_t FIRE_SPARK_NOISE_SCALE2 = 450;
  static const uint16_t FIRE_SPARK_NOISE_SPEED2 = 17;
  static const uint8_t  FIRE_SPARK_OCTAVE_BLEND = 128; // 0..255 blend between octaves
  static const uint16_t FIRE_SPARK_DRIFT_SPEED  = 3;
  static const uint8_t  FIRE_SPARK_MIN_PROB     = 1;   // out of 255
  static const uint8_t  FIRE_SPARK_MAX_PROB     = 10;  // out of 255
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MIN = 150;
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MAX = 240;

  // Partial diffusion weight (0..255). Smaller preserves current heat more.
  static const uint8_t  FIRE_DIFFUSE_WEIGHT = 96;

  // Fire amber glow base layer
  static const uint8_t  GLOW_HUE         = 31;
  static const uint8_t  GLOW_SAT         = 200;
  static const uint8_t  GLOW_VAL_MIN     = 90;
  static const uint8_t  GLOW_VAL_MAX     = 180;
  static const uint8_t  GLOW_TRACK_WEIGHT= 16;

  // Folder 3: Christmas calm alternating pulse
  static const uint8_t  XMAS_MIN_BRIGHT = 30;
  static const uint8_t  XMAS_MAX_BRIGHT = 255;
  static const uint8_t  XMAS_PULSE_BPM  = 18;

  // Folder 4: Spooky (favor green) with slow breathing pulse
  static const uint8_t  SPOOKY_PULSE_BPM        = 12;
  static const uint16_t SPOOKY_TIME_NOISE_SPEED = 1;
  static const uint16_t SPOOKY_COLUMN_NOISE_SCALE = 5000;

  // ============================================================
  // Public API
  // ============================================================

  // LED buffer (FastLED)
  extern CRGB leds[NUM_LEDS];

  // Initialize FastLED and clear the matrix
  void begin();

  // Clear internal buffer to black (does not call FastLED.show()).
  void clear();

  // Non-blocking update. Renders at most once per FRAME_INTERVAL_MS.
  //
  // folder:
  //   1..4 = render corresponding theme
  //   99   = treat as "off" (clear + latch)
  //
  // lightsOn:
  //   true  = render normally
  //   false = treat as "off" (clear + latch)
  void update(uint8_t folder, bool lightsOn);

} // namespace LedMatrix

#endif // LEDMATRIX_
