
// LedMatrix.h
#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <FastLED.h>

namespace LedMatrix {
  // ================== Hardware & global settings ==================
  static const uint8_t  DATA_PIN   = 6;   // WS2812B DIN (through 330–470Ω recommended)
  static const uint16_t MATRIX_WIDTH  = 32; // columns
  static const uint16_t MATRIX_HEIGHT = 8;  // rows
  static const uint16_t NUM_LEDS      = MATRIX_WIDTH * MATRIX_HEIGHT; // 256
  // WS2812B is typically GRB
  static const EOrder   COLOR_ORDER = GRB;
  // Global brightness (you chose 50)
  static const uint8_t  BRIGHTNESS  = 50;
  // Baseline frame pacing (non-blocking). Lower = faster updates.
  static const uint16_t FRAME_INTERVAL_MS = 15;

  // ================== Folder-specific parameters ==================
  // Folder 1: Party marble (relaxed)
  static const uint16_t PARTY_COLUMN_NOISE_SCALE = 4000; // larger = smoother between columns
  static const uint16_t PARTY_TIME_NOISE_SPEED   = 3;    // smaller = slower evolution

  // Folder 2: Fire columns (flickery). One heat value per column.
  static const uint8_t  FIRE_COOLING           = 55;  // cooling per frame (higher = more cooling)
  static const uint8_t  FIRE_BASE_BRIGHTNESS   = 255; // brightness used with HeatColors_p
  static const uint8_t  FIRE_FLICKER_VARIANCE  = 30;  // extra brightness jitter
  static const uint8_t  FIRE_SPEED_SCALE       = 1;   // 1 = baseline; increase to speed updates

  // Extra edge cooling & attenuation (non-spark; keep if you want cooler ends)
  static const uint8_t  FIRE_EDGE_EXTRA_COOLING      = 6;    // additional cooling near edges
  static const uint8_t  FIRE_EDGE_ATTENUATION        = 170;  // scale8 factor (~66%) after diffusion
  static const uint8_t  FIRE_EDGE_ATTENUATION_WIDTH  = 3;    // how many columns are attenuated at each edge

  // Global (anywhere) sparks driven by multi-octave noise with horizontal drift
  static const uint16_t FIRE_SPARK_NOISE_SCALE       = 1800; // octave 1: broad, smooth
  static const uint16_t FIRE_SPARK_NOISE_SPEED       = 9;    // octave 1: time evolution
  static const uint16_t FIRE_SPARK_NOISE_SCALE2      = 450;  // octave 2: finer detail
  static const uint16_t FIRE_SPARK_NOISE_SPEED2      = 17;   // octave 2: faster evolution
  static const uint8_t  FIRE_SPARK_OCTAVE_BLEND      = 128;  // mix weight 0..255 for octave 2 (~50%)
  static const uint16_t FIRE_SPARK_DRIFT_SPEED       = 3;    // horizontal drift speed (pixels per frame)
  static const uint8_t  FIRE_SPARK_MIN_PROB          = 1;    // min probability out of 255
  static const uint8_t  FIRE_SPARK_MAX_PROB          = 10;   // max probability out of 255 (kept modest)
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MIN   = 150;  // anywhere spark heat range
  static const uint8_t  FIRE_GLOBAL_SPARK_HEAT_MAX   = 240;

  // Partial diffusion weight (0..255). 128 ≈ 50/50. Smaller preserves current heat more.
  static const uint8_t  FIRE_DIFFUSE_WEIGHT          = 96;   // ~37.5% neighbor / 62.5% self

  // Folder 3: Christmas calm alternating pulse
  static const uint8_t  XMAS_MIN_BRIGHT = 30;  // dim level
  static const uint8_t  XMAS_MAX_BRIGHT = 255; // bright level
  static const uint8_t  XMAS_PULSE_BPM  = 18;  // slow pulse (beats per minute)

  // Folder 4: Spooky (favor green) with slow breathing pulse
  static const uint8_t  SPOOKY_PULSE_BPM        = 12;   // very slow breathing
  static const uint16_t SPOOKY_TIME_NOISE_SPEED = 1;    // gentle evolution
  static const uint16_t SPOOKY_COLUMN_NOISE_SCALE = 5000; // smooth columns

  // ================== Public API ==================
  extern CRGB leds[NUM_LEDS];
  void begin();         // init FastLED and buffers
  void clear();         // clear to black
  // Main update: decides which effect to render based on folder & on/off flag.
  // Non-blocking: respects FRAME_INTERVAL_MS.
  void update(uint8_t folder, bool lightsOn);
}

#endif // LEDMATRIX_H
