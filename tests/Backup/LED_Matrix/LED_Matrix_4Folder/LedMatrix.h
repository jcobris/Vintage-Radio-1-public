
// LedMatrix.h
#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <FastLED.h>

namespace LedMatrix {
  // ================== Hardware & global settings ==================
  static const uint8_t  DATA_PIN          = 6;       // WS2812B DIN (through 330–470Ω recommended)
  static const uint16_t MATRIX_WIDTH      = 32;      // columns
  static const uint16_t MATRIX_HEIGHT     = 8;       // rows
  static const uint16_t NUM_LEDS          = MATRIX_WIDTH * MATRIX_HEIGHT; // 256

  // WS2812B is typically GRB
  static const EOrder   COLOR_ORDER       = GRB;

  // Global brightness
  static const uint8_t  BRIGHTNESS        = 50;

  // Baseline frame pacing (non-blocking). Lower = faster updates.
  static const uint16_t FRAME_INTERVAL_MS = 15;

  // ================== Folder-specific parameters ==================
  // Folder 1: Party marble (relaxed)
  static const uint16_t PARTY_COLUMN_NOISE_SCALE = 4000; // larger = smoother between columns
  static const uint16_t PARTY_TIME_NOISE_SPEED   = 3;    // smaller = slower evolution

  // ===== Folder 2: Fire columns (two-front inward flames, red-biased, warm glow) =====
  static const uint8_t  FIRE_COOLING             = 50;   // cooling per frame (higher = more cooling)
  static const uint8_t  FIRE_SPARKING            = 100;  // base spark probability (per side)
  static const uint8_t  FIRE_SPEED_SCALE         = 1;    // 1 = baseline frame rate

  // Spark injection near ends (independent per side)
  static const uint8_t  FIRE_SPARK_HEAT_MIN      = 160;  // heat added on spark (min)
  static const uint8_t  FIRE_SPARK_HEAT_MAX      = 255;  // heat added on spark (max)
  static const uint8_t  FIRE_SPARK_ZONE_WIDTH    = 2;    // columns near each end used for injection

  // Flame brightness scales with heat so flame is visible at low heat
  static const uint8_t  FIRE_FLAME_BRIGHT_MIN    = 150;
  static const uint8_t  FIRE_FLAME_BRIGHT_MAX    = 255;

  // Background glow (slightly brighter, less yellow to avoid washout)
  static const uint8_t  FIRE_GLOW_HUE            = 31;   // deeper orange (vs 35)
  static const uint8_t  FIRE_GLOW_SAT            = 190;  // slightly less saturated than 200
  static const uint8_t  FIRE_GLOW_MIN_VAL        = 90;   // brighter base glow
  static const uint8_t  FIRE_GLOW_MAX_VAL        = 115;  // but capped to not overwhelm high heat

  // Blend control: low heat -> glow-dominant, high heat -> flame-dominant
  static const uint8_t  FIRE_BLEND_MIN_AMOUNT    = 42;   // more flame even at low heat
  static const uint8_t  FIRE_BLEND_MAX_AMOUNT    = 245;  // strong flame at high heat

  // Keep HeatColors_p but cap to avoid constant yellow/white wash
  static const uint8_t  FIRE_PALETTE_CAP         = 202;  // lower cap: reds/oranges dominate, less white

  // Flicker control (spatial + temporal noise) — calmer sideways bursts
  static const uint16_t FIRE_FLICKER_SPATIAL_SCALE = 150; // broader bands across columns
  static const uint16_t FIRE_FLICKER_TIME_SPEED    = 20;  // slower evolution
  static const uint8_t  FIRE_FLICKER_DEPTH         = 24;  // reduced modulation depth

  // Red bias controls (global tint toward red)
  static const uint8_t  FIRE_RED_IDX_BIAS         = 10;   // subtract from palette index (toward reds)
  static const uint8_t  FIRE_RED_CHANNEL_BOOST    = 20;   // add to red channel
  static const uint8_t  FIRE_GREEN_CHANNEL_REDUCE = 10;   // subtract from green channel

  // Folder 3: Christmas calm alternating pulse
  static const uint8_t  XMAS_MIN_BRIGHT          = 30;   // dim level
  static const uint8_t  XMAS_MAX_BRIGHT          = 255;  // bright level
  static const uint8_t  XMAS_PULSE_BPM           = 18;   // slow pulse (beats per minute)

  // Folder 4: Spooky (favor green) with slow breathing pulse
  static const uint8_t  SPOOKY_PULSE_BPM         = 12;   // very slow breathing
  static const uint16_t SPOOKY_TIME_NOISE_SPEED  = 1;    // gentle evolution
  static const uint16_t SPOOKY_COLUMN_NOISE_SCALE = 5000; // smooth columns

  // ================== Public API ==================
  extern CRGB leds[NUM_LEDS];

  void begin();                       // init FastLED and buffers
  void clear();                       // clear to black

  // Main update: decides which effect to render based on folder & on/off flag.
  // Non-blocking: respects FRAME_INTERVAL_MS.
  void update(uint8_t folder, bool lightsOn);
}

#endif // LEDMATRIX_H
