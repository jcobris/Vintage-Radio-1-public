
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

 Key constraint:
  - One solid color per column (8 LEDs per column are identical).
    Each renderer computes one CRGB per column and applies it to the 8 LEDs.

 Operating modes:
  - Folder 1: Party / rainbow marble
  - Folder 2: Fire (one heat value per column, plus glow base)
  - Folder 3: Christmas (left half red pulse, right half green pulse)
  - Folder 4: Spooky (green/aqua marbling with breathing pulse)

 Brightness:
  - Global FastLED brightness is controlled by the main sketch via FastLED.setBrightness().
    This module does not own brightness settings to avoid multiple sources of truth.

 Update model:
  - LedMatrix::update(folder, lightsOn) is non-blocking and frame-throttled.
  - If lightsOn is false OR folder==99, the matrix is cleared and latched OFF.

 Sync support:
  - setSpookyBreath(pulse) lets the main sketch provide a shared breath value so
    the matrix and dial LED can breathe in perfect sync.
*/

namespace LedMatrix {

  // Hardware & geometry
  static const uint8_t  DATA_PIN      = Config::PIN_MATRIX_DATA;
  static const uint16_t MATRIX_WIDTH  = 32; // columns
  static const uint16_t MATRIX_HEIGHT = 8;  // rows
  static const uint16_t NUM_LEDS      = MATRIX_WIDTH * MATRIX_HEIGHT; // 256
  static const EOrder   COLOR_ORDER   = GRB;

  // Frame pacing
  static const uint16_t FRAME_INTERVAL_MS = 30;

  // Folder 1: Party marble (relaxed)
  static const uint16_t PARTY_COLUMN_NOISE_SCALE = 4000;
  static const uint16_t PARTY_TIME_NOISE_SPEED   = 3;

  // Folder 2: Fire columns
  static const uint8_t  FIRE_COOLING             = 55;
  static const uint8_t  FIRE_BASE_BRIGHTNESS     = 255;
  static const uint8_t  FIRE_FLICKER_VARIANCE    = 8;
  static const uint8_t  FIRE_FLICKER_PERIOD      = 16;
  static const uint8_t  FIRE_SPEED_SCALE         = 1;
  static const uint16_t FIRE_SPARK_NOISE_SCALE   = 1800;
  static const uint16_t FIRE_SPARK_NOISE_SPEED   = 9;
  static const uint16_t FIRE_SPARK_NOISE_SCALE2  = 450;
  static const uint16_t FIRE_SPARK_NOISE_SPEED2  = 17;
  static const uint8_t  FIRE_SPARK_OCTAVE_BLEND  = 128;
  static const uint16_t FIRE_SPARK_DRIFT_SPEED   = 3;
  static const uint8_t  FIRE_SPARK_MIN_PROB      = 1;
  static const uint8_t  FIRE_SPARK_MAX_PROB      = 10;
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MIN = 150;
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MAX = 240;
  static const uint8_t  FIRE_DIFFUSE_WEIGHT      = 96;

  static const uint8_t  GLOW_HUE                 = 31;
  static const uint8_t  GLOW_SAT                 = 200;
  static const uint8_t  GLOW_VAL_MIN             = 90;
  static const uint8_t  GLOW_VAL_MAX             = 180;
  static const uint8_t  GLOW_TRACK_WEIGHT        = 16;

  // Folder 3: Christmas calm alternating pulse
  static const uint8_t  XMAS_MIN_BRIGHT          = 30;
  static const uint8_t  XMAS_MAX_BRIGHT          = 255;
  static const uint8_t  XMAS_PULSE_BPM           = 18;

  // Folder 4: Spooky marbling field
  static const uint16_t SPOOKY_TIME_NOISE_SPEED   = 1;
  static const uint16_t SPOOKY_COLUMN_NOISE_SCALE = 5000;

  // LED buffer
  extern CRGB leds[NUM_LEDS];

  // API
  void begin();
  void clear();
  void update(uint8_t folder, bool lightsOn);

  // Provide an external breath brightness (0..255) used by Folder 4 renderer.
  // Pass 0xFF to release and use internal fallback breathing.
  void setSpookyBreath(uint8_t pulse);
}

#endif // LEDMATRIX_H
