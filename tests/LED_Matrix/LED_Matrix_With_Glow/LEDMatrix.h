// LEDMatrix.h
#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <FastLED.h>

namespace LedMatrix {
  // ================== Hardware & global settings ==================
  static const uint8_t  DATA_PIN   = 7;   // WS2812B DIN (through 330–470Ω recommended)
  static const uint16_t MATRIX_WIDTH  = 32; // columns
  static const uint16_t MATRIX_HEIGHT = 8;  // rows
  static const uint16_t NUM_LEDS      = MATRIX_WIDTH * MATRIX_HEIGHT; // 256
  static const EOrder   COLOR_ORDER   = GRB;
  static const uint8_t  BRIGHTNESS    = 50;
  static const uint16_t FRAME_INTERVAL_MS = 15;

  // ================== Folder-specific parameters ==================
  // Folder 1: Party marble (relaxed)
  static const uint16_t PARTY_COLUMN_NOISE_SCALE = 4000;
  static const uint16_t PARTY_TIME_NOISE_SPEED   = 3;

  // Folder 2: Fire columns (flickery). One heat value per column.
  static const uint8_t  FIRE_COOLING           = 55;
  static const uint8_t  FIRE_BASE_BRIGHTNESS   = 255;

  // ---- Flicker (calmer) ----
  static const uint8_t  FIRE_FLICKER_VARIANCE  = 8;    // smaller jitter (0 disables)
  static const uint8_t  FIRE_FLICKER_PERIOD    = 16;   // fewer updates

  static const uint8_t  FIRE_SPEED_SCALE       = 1;

  // Global (anywhere) sparks driven by multi-octave noise with horizontal drift
  static const uint16_t FIRE_SPARK_NOISE_SCALE       = 1800;
  static const uint16_t FIRE_SPARK_NOISE_SPEED       = 9;
  static const uint16_t FIRE_SPARK_NOISE_SCALE2      = 450;
  static const uint16_t FIRE_SPARK_NOISE_SPEED2      = 17;
  static const uint8_t  FIRE_SPARK_OCTAVE_BLEND      = 128;  // 0..255
  static const uint16_t FIRE_SPARK_DRIFT_SPEED       = 3;
  static const uint8_t  FIRE_SPARK_MIN_PROB          = 1;    // out of 255
  static const uint8_t  FIRE_SPARK_MAX_PROB          = 10;   // out of 255
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MIN   = 150;
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MAX   = 240;

  // Partial diffusion weight (0..255). 128 ≈ 50/50. Smaller preserves current heat more.
  static const uint8_t  FIRE_DIFFUSE_WEIGHT          = 96;

  // ---- Amber glow (base layer) ----
  static const uint8_t  GLOW_HUE         = 31;   // warm amber/orange
  static const uint8_t  GLOW_SAT         = 200;  // fairly saturated
  static const uint8_t  GLOW_VAL_MIN     = 90;   // brighter floor for visibility
  static const uint8_t  GLOW_VAL_MAX     = 180;  // modest ceiling (below flame highlights)
  static const uint8_t  GLOW_TRACK_WEIGHT= 16;   // smoothing for “tube-warm” feel

  // Folder 3: Christmas calm alternating pulse
  static const uint8_t  XMAS_MIN_BRIGHT = 30;
  static const uint8_t  XMAS_MAX_BRIGHT = 255;
  static const uint8_t  XMAS_PULSE_BPM  = 18;

  // Folder 4: Spooky (favor green) with slow breathing pulse
  static const uint8_t  SPOOKY_PULSE_BPM        = 12;
  static const uint16_t SPOOKY_TIME_NOISE_SPEED = 1;
  static const uint16_t SPOOKY_COLUMN_NOISE_SCALE = 5000;

  // ================== Public API ==================
  extern CRGB leds[NUM_LEDS];
  void begin();         // init FastLED and buffers
  void clear();         // clear to black
  void update(uint8_t folder, bool lightsOn); // non-blocking; uses FRAME_INTERVAL_MS
}

#endif // LEDMATRIX_H