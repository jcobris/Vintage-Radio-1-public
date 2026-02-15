
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

  // ===== Folder 2: Fire (global noise-driven fire, red-biased, fast/flickery) =====
  // Column-wise noise: use X-scale to control spatial smoothness across 32 columns.
  static const uint16_t FIRE_NOISE_COLUMN_SCALE  = 4800; // larger = smoother between columns

  // Time evolution speed: higher = faster fire motion
  static const uint16_t FIRE_NOISE_TIME_SPEED    = 24;   // fast evolution

  // Flicker modulation (spatial + temporal): wider bands, calmer sideways bursts
  static const uint16_t FIRE_FLICKER_SPATIAL_SCALE = 90; // noise scale across columns for flicker
  static const uint16_t FIRE_FLICKER_TIME_SPEED    = 28; // flicker evolution speed
  static const uint8_t  FIRE_FLICKER_DEPTH         = 26; // how much flicker nudges brightness/index

  // Flame brightness range (applied after noise/flicker)
  static const uint8_t  FIRE_FLAME_BRIGHT_MIN     = 150;
  static const uint8_t  FIRE_FLAME_BRIGHT_MAX     = 255;

  // Palette cap (reduce constant yellow/white); whites still possible via rare spark overlay
  static const uint8_t  FIRE_PALETTE_CAP          = 200; // red/orange dominance

  // Red bias controls (global tint toward red)
  static const uint8_t  FIRE_RED_IDX_BIAS         = 8;   // subtract from palette index (toward reds)
  static const uint8_t  FIRE_RED_CHANNEL_BOOST    = 18;  // add to red channel
  static const uint8_t  FIRE_GREEN_CHANNEL_REDUCE = 8;   // subtract from green channel

  // Rare white spark overlay (away from edges only)
  static const uint8_t  FIRE_WHITE_SPARK_CHANCE   = 3;   // out of 255 (~1.2% chance per column/frame)
  static const uint8_t  FIRE_WHITE_SPARK_IDX_MIN  = 208; // only when index is already near the top
  static const uint8_t  FIRE_WHITE_EDGE_GUARD     = 2;   // no white sparks in outer N columns

  // Optional subtle background glow (kept low to avoid washing out red)
  static const uint8_t  FIRE_GLOW_ENABLE          = 1;   // 1=on, 0=off
  static const uint8_t  FIRE_GLOW_HUE             = 31;  // deeper orange
  static const uint8_t  FIRE_GLOW_SAT             = 180; // slightly reduced saturation
  static const uint8_t  FIRE_GLOW_MIN_VAL         = 70;  // dim base glow
  static const uint8_t  FIRE_GLOW_MAX_VAL         = 100; // capped glow so flame remains dominant

  // Blend control: low heat/index -> glow-dominant, high -> flame-dominant (only relevant if glow enabled)
  static const uint8_t  FIRE_BLEND_MIN_AMOUNT     = 40;  // more flame even at low
  static const uint8_t  FIRE_BLEND_MAX_AMOUNT     = 245; // strong flame at high

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
