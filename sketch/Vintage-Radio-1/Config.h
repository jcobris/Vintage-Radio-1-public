
// Config.h
#pragma once
#include <Arduino.h>
/*
============================================================
Project Configuration (Single Source of Truth)
============================================================
This header defines:
 - Compile-time debug macros (global + per-module toggles)
 - Pin assignments (Arduino Nano / ATmega328P)
 - Baud rates for SoftwareSerial UARTs (BT + MP3)
 - Small constants used across modules
 - Compile-time pin conflict guards (static_assert)
Notes:
 - Nano has 2KB SRAM; avoid dynamic allocation and large buffers.
 - static_assert guards are compile-time only (no runtime SRAM cost).
*/

// ============================================================
// Global debug control
// ============================================================
// DEBUG is the master switch.
// - DEBUG=0 compiles *all* debug output out.
// - DEBUG=1 allows debug output, further filtered by per-module flags below.
#ifndef DEBUG
 #define DEBUG 0
#endif
#if DEBUG == 1
 #define debug(x) Serial.print(x)
 #define debugln(x) Serial.println(x)
#else
 #define debug(x) do {} while (0)
 #define debugln(x) do {} while (0)
#endif

// ============================================================
// Per-module debug toggles (simple "set to 1" style)
// ============================================================
// Set each item to 1/0 as desired.
// These are compile-time constants; when off, code compiles out.
#ifndef BOOT_DEBUG
 #define BOOT_DEBUG 0
#endif
#ifndef SRC_DEBUG
 #define SRC_DEBUG 0
#endif
#ifndef TUNING_DEBUG
 #define TUNING_DEBUG 0
#endif
#ifndef TUNING_STREAM_DEBUG
 #define TUNING_STREAM_DEBUG 0 // VERY noisy continuous timing output
#endif
#ifndef TUNING_VERBOSE_DEBUG
 #define TUNING_VERBOSE_DEBUG 0 // samples, hit counts, extra detail
#endif
#ifndef MP3_DEBUG
 #define MP3_DEBUG 0
#endif
#ifndef MP3_FRAME_DEBUG
 #define MP3_FRAME_DEBUG 0 // TX frame hex dumps (noisy)
#endif
#ifndef MP3_RX_DEBUG
 #define MP3_RX_DEBUG 0 // RX byte dumps (noisy)
#endif
#ifndef BT_DEBUG
 #define BT_DEBUG 0
#endif
#ifndef LED_STRIP_DEBUG
 #define LED_STRIP_DEBUG 0
#endif
#ifndef LED_MATRIX_DEBUG
 #define LED_MATRIX_DEBUG 0
#endif
#ifndef LED_DIAL_DEBUG
 #define LED_DIAL_DEBUG 0
#endif

// ============================================================
// Debug macros per module (flash-string friendly)
// ============================================================
// Convention:
// - DBG_xxx(x) expects a single printable item, typically F("...") or a value.
// - DBG_xxx2(a,b) prints two parts without building Strings.
// - DBG_xxx_KV(keyF, val) prints "key" then value with newline.
#if (DEBUG == 1) && (BOOT_DEBUG == 1)
 #define DBG_BOOT(x) debugln(x)
 #define DBG_BOOT2(a,b) do { debug(a); debugln(b); } while (0)
 #define DBG_BOOT_KV(kF,v) do { debug(kF); debugln(v); } while (0)
#else
 #define DBG_BOOT(x) do {} while (0)
 #define DBG_BOOT2(a,b) do {} while (0)
 #define DBG_BOOT_KV(kF,v) do {} while (0)
#endif

#if (DEBUG == 1) && (SRC_DEBUG == 1)
 #define DBG_SRC(x) debugln(x)
 #define DBG_SRC2(a,b) do { debug(a); debugln(b); } while (0)
 #define DBG_SRC_KV(kF,v) do { debug(kF); debugln(v); } while (0)
#else
 #define DBG_SRC(x) do {} while (0)
 #define DBG_SRC2(a,b) do {} while (0)
 #define DBG_SRC_KV(kF,v) do {} while (0)
#endif

#if (DEBUG == 1) && (TUNING_DEBUG == 1)
 #define DBG_TUNING(x) debugln(x)
 #define DBG_TUNING2(a,b) do { debug(a); debugln(b); } while (0)
 #define DBG_TUNING_KV(kF,v) do { debug(kF); debugln(v); } while (0)
#else
 #define DBG_TUNING(x) do {} while (0)
 #define DBG_TUNING2(a,b) do {} while (0)
 #define DBG_TUNING_KV(kF,v) do {} while (0)
#endif

#if (DEBUG == 1) && (TUNING_STREAM_DEBUG == 1)
 #define DBG_TUNING_STREAM(x) debugln(x)
 #define DBG_TUNING_STREAM2(a,b) do { debug(a); debugln(b); } while (0)
#else
 #define DBG_TUNING_STREAM(x) do {} while (0)
 #define DBG_TUNING_STREAM2(a,b) do {} while (0)
#endif

#if (DEBUG == 1) && (TUNING_VERBOSE_DEBUG == 1)
 #define DBG_TUNING_VERBOSE(x) debugln(x)
 #define DBG_TUNING_VERBOSE2(a,b) do { debug(a); debugln(b); } while (0)
#else
 #define DBG_TUNING_VERBOSE(x) do {} while (0)
 #define DBG_TUNING_VERBOSE2(a,b) do {} while (0)
#endif

#if (DEBUG == 1) && (MP3_DEBUG == 1)
 #define DBG_MP3(x) debugln(x)
 #define DBG_MP32(a,b) do { debug(a); debugln(b); } while (0)
 #define DBG_MP3_KV(kF,v) do { debug(kF); debugln(v); } while (0)
#else
 #define DBG_MP3(x) do {} while (0)
 #define DBG_MP32(a,b) do {} while (0)
 #define DBG_MP3_KV(kF,v) do {} while (0)
#endif

#if (DEBUG == 1) && (MP3_FRAME_DEBUG == 1)
 #define DBG_MP3_FRAME(x) debugln(x)
#else
 #define DBG_MP3_FRAME(x) do {} while (0)
#endif

#if (DEBUG == 1) && (MP3_RX_DEBUG == 1)
 #define DBG_MP3_RX(x) debugln(x)
#else
 #define DBG_MP3_RX(x) do {} while (0)
#endif

#if (DEBUG == 1) && (BT_DEBUG == 1)
 #define DBG_BT(x) debugln(x)
 #define DBG_BT2(a,b) do { debug(a); debugln(b); } while (0)
#else
 #define DBG_BT(x) do {} while (0)
 #define DBG_BT2(a,b) do {} while (0)
#endif

#if (DEBUG == 1) && (LED_STRIP_DEBUG == 1)
 #define DBG_LED_STRIP(x) debugln(x)
 #define DBG_LED_STRIP2(a,b) do { debug(a); debugln(b); } while (0)
#else
 #define DBG_LED_STRIP(x) do {} while (0)
 #define DBG_LED_STRIP2(a,b) do {} while (0)
#endif

#if (DEBUG == 1) && (LED_MATRIX_DEBUG == 1)
 #define DBG_LED_MATRIX(x) debugln(x)
#else
 #define DBG_LED_MATRIX(x) do {} while (0)
#endif

#if (DEBUG == 1) && (LED_DIAL_DEBUG == 1)
 #define DBG_LED_DIAL(x) debugln(x)
#else
 #define DBG_LED_DIAL(x) do {} while (0)
#endif

// ============================================================
// Pinout + constants (Arduino Nano / ATmega328P)
// ============================================================
namespace Config {
  // Source detect input (physical source select switch)
  // Pin mode: INPUT_PULLUP
  // LOW -> MP3 selected
  // HIGH -> Bluetooth selected
  constexpr uint8_t PIN_SOURCE_DETECT = 2; // D2

  // 3-position display mode switch (3 poles to GND)
  // Pin mode: INPUT_PULLUP, LOW = selected
  constexpr uint8_t PIN_DISPLAY_MODE_NORMAL = 6; // D6
  constexpr uint8_t PIN_DISPLAY_MODE_ALT    = 4; // D4
  constexpr uint8_t PIN_DISPLAY_MODE_OFF    = 5; // D5

  // Dial / tuner illumination LED (PWM)
  constexpr uint8_t PIN_LED_DISPLAY = 3; // D3

  // WS2812B 8x32 LED matrix data pin
  constexpr uint8_t PIN_MATRIX_DATA = 7; // D7

  // Tuning input (RC timing input)
  constexpr uint8_t PIN_TUNING_INPUT = 8; // D8

  // Bluetooth module UART (SoftwareSerial)
  constexpr uint8_t PIN_BT_RX = 10; // D10 (Arduino RX for BT TX)
  constexpr uint8_t PIN_BT_TX = 9;  // D9  (Arduino TX for BT RX)
  constexpr long BT_BAUD = 57600;

  // MP3 module UART (SoftwareSerial)
  constexpr uint8_t PIN_MP3_RX = 12; // D12 (Arduino RX for MP3 TXD)
  constexpr uint8_t PIN_MP3_TX = 11; // D11 (Arduino TX for MP3 RXD)
  constexpr long MP3_BAUD = 9600;

  // Next-track pushbutton (active-low to GND)
  constexpr uint8_t PIN_NEXT_TRACK_BUTTON = 13; // D13

  // WS2812B LED strip on A0 (data)
  constexpr uint8_t PIN_STRIP_DATA = A0; // A0

  // UPDATED: 128 LEDs in the strip
  constexpr uint16_t STRIP_NUM_LEDS = 90;
}

// ============================================================
// Compile-time pin conflict guards
// ============================================================
#define CONFIG_ASSERT_PIN_DISTINCT(a, b, msg) static_assert((a) != (b), msg)
#define CONFIG_ASSERT_NOT_SERIAL_PINS(p, msg) static_assert(((p) != 0) && ((p) != 1), msg)

// ---- Basic internal pin sanity ----
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_BT_RX, Config::PIN_BT_TX, "Pin conflict: BT RX and BT TX must differ.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_MP3_RX, Config::PIN_MP3_TX, "Pin conflict: MP3 RX and MP3 TX must differ.");

// ---- Avoid using Serial pins where inappropriate ----
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_TUNING_INPUT, "Invalid pin: tuning input must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_MATRIX_DATA, "Invalid pin: matrix data must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_LED_DISPLAY, "Invalid pin: dial LED must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_NEXT_TRACK_BUTTON, "Invalid pin: next-track button must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_STRIP_DATA, "Invalid pin: strip data must not use D0/D1 (Serial).");

// ---- Dial LED vs serial subsystems ----
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_LED_DISPLAY, Config::PIN_MP3_TX, "Pin conflict: dial LED pin conflicts with MP3 TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_LED_DISPLAY, Config::PIN_MP3_RX, "Pin conflict: dial LED pin conflicts with MP3 RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_LED_DISPLAY, Config::PIN_BT_TX,  "Pin conflict: dial LED pin conflicts with BT TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_LED_DISPLAY, Config::PIN_BT_RX,  "Pin conflict: dial LED pin conflicts with BT RX.");

// ---- Matrix vs serial subsystems ----
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_MATRIX_DATA, Config::PIN_MP3_TX, "Pin conflict: matrix pin conflicts with MP3 TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_MATRIX_DATA, Config::PIN_MP3_RX, "Pin conflict: matrix pin conflicts with MP3 RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_MATRIX_DATA, Config::PIN_BT_TX,  "Pin conflict: matrix pin conflicts with BT TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_MATRIX_DATA, Config::PIN_BT_RX,  "Pin conflict: matrix pin conflicts with BT RX.");

// ---- Matrix vs dial LED ----
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_MATRIX_DATA, Config::PIN_LED_DISPLAY, "Pin conflict: matrix and dial LED pins must differ.");

// ---- Next-track button pin guards ----
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_SOURCE_DETECT,       "Pin conflict: next-track button conflicts with source detect.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_DISPLAY_MODE_NORMAL, "Pin conflict: next-track button conflicts with display mode NORMAL.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_DISPLAY_MODE_ALT,    "Pin conflict: next-track button conflicts with display mode ALT.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_DISPLAY_MODE_OFF,    "Pin conflict: next-track button conflicts with display mode OFF.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_BT_RX,               "Pin conflict: next-track button conflicts with BT RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_BT_TX,               "Pin conflict: next-track button conflicts with BT TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_MP3_RX,              "Pin conflict: next-track button conflicts with MP3 RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_MP3_TX,              "Pin conflict: next-track button conflicts with MP3 TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_TUNING_INPUT,        "Pin conflict: next-track button conflicts with tuning input.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_LED_DISPLAY,         "Pin conflict: next-track button conflicts with dial LED.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_MATRIX_DATA,         "Pin conflict: next-track button conflicts with matrix data.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_NEXT_TRACK_BUTTON, Config::PIN_STRIP_DATA,          "Pin conflict: next-track button conflicts with strip data.");

// ---- Strip vs other subsystems ----
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_MATRIX_DATA, "Pin conflict: strip data conflicts with matrix data.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_LED_DISPLAY, "Pin conflict: strip data conflicts with dial LED.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_MP3_RX,      "Pin conflict: strip data conflicts with MP3 RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_MP3_TX,      "Pin conflict: strip data conflicts with MP3 TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_BT_RX,       "Pin conflict: strip data conflicts with BT RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_BT_TX,       "Pin conflict: strip data conflicts with BT TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_SOURCE_DETECT,"Pin conflict: strip data conflicts with source detect.");

// Clean up helper macros to avoid leaking to other headers.
#undef CONFIG_ASSERT_PIN_DISTINCT
#undef CONFIG_ASSERT_NOT_SERIAL_PINS
