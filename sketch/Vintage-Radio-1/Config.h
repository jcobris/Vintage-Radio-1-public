
// Config.h
#pragma once
#include <Arduino.h>

/*
 ============================================================
 Project Configuration (Single Source of Truth)
 ============================================================

 This header defines:
  - Compile-time debug macros (debug / debugln)
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
// Set DEBUG=1 to enable Serial debug prints where supported.
// Default DEBUG=0 keeps the build quiet and reduces runtime overhead.
//
// You can override this at compile time or by defining DEBUG before
// including Config.h in a translation unit (not recommended unless deliberate).
#ifndef DEBUG
  #define DEBUG 1
#endif

#if DEBUG == 1
  #define debug(x)   Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debug(x)   do {} while (0)
  #define debugln(x) do {} while (0)
#endif

// ============================================================
// Pinout + constants (Arduino Nano / ATmega328P)
// ============================================================
namespace Config {

  // ----------------------------------------------------------
  // Source detect input (physical source select switch)
  // ----------------------------------------------------------
  // Pin mode: INPUT_PULLUP
  // LOW  -> MP3 selected
  // HIGH -> Bluetooth selected
  constexpr uint8_t PIN_SOURCE_DETECT = 2; // D2

  // ----------------------------------------------------------
  // 3-position display mode switch (3 poles to GND)
  // ----------------------------------------------------------
  // Wiring: each pole grounds one pin when selected.
  // Pin mode: INPUT_PULLUP, LOW = selected
  //
  // NORMAL: Normal theme behaviour
  // ALT:    Reserved; currently same as normal
  // OFF:    Matrix OFF; dial LED forced solid ON
  constexpr uint8_t PIN_DISPLAY_MODE_NORMAL = 3; // D3
  constexpr uint8_t PIN_DISPLAY_MODE_ALT    = 4; // D4
  constexpr uint8_t PIN_DISPLAY_MODE_OFF    = 5; // D5

  // ----------------------------------------------------------
  // Dial / tuner illumination LED string (PWM)
  // ----------------------------------------------------------
  constexpr uint8_t PIN_LED_DISPLAY = 6; // D6

  // ----------------------------------------------------------
  // WS2812B 8x32 LED matrix data pin
  // ----------------------------------------------------------
  constexpr uint8_t PIN_MATRIX_DATA = 7; // D7

  // ----------------------------------------------------------
  // Tuning input (RC timing input)
  // ----------------------------------------------------------
  // IMPORTANT: Avoid D0/D1 because those pins are used by hardware Serial.
  constexpr uint8_t PIN_TUNING_INPUT = 8; // D8

  // ----------------------------------------------------------
  // Bluetooth module (BT201) UART (SoftwareSerial)
  // ----------------------------------------------------------
  // RX/TX naming is from Arduino’s perspective:
  // - PIN_BT_RX is the Arduino RX pin (connects to BT TX)
  // - PIN_BT_TX is the Arduino TX pin (connects to BT RX)
  constexpr uint8_t PIN_BT_RX = 10; // D10 (Arduino RX for BT TX)
  constexpr uint8_t PIN_BT_TX = 9;  // D9  (Arduino TX for BT RX)
  constexpr long    BT_BAUD   = 57600;

  // ----------------------------------------------------------
  // MP3 module (DY-SV5W) UART (SoftwareSerial)
  // ----------------------------------------------------------
  // RX/TX naming is from Arduino’s perspective:
  // - PIN_MP3_RX is the Arduino RX pin (connects to MP3 TXD)
  // - PIN_MP3_TX is the Arduino TX pin (connects to MP3 RXD)
  constexpr uint8_t PIN_MP3_RX = 12; // D12 (Arduino RX for MP3 TXD)
  constexpr uint8_t PIN_MP3_TX = 11; // D11 (Arduino TX for MP3 RXD)
  constexpr long    MP3_BAUD   = 9600;

  // ----------------------------------------------------------
  // Next-track pushbutton (active-low to GND)
  // ----------------------------------------------------------
  // Pin mode: INPUT_PULLUP
  // LOW -> button pressed -> send MP3 next-track command
  constexpr uint8_t PIN_NEXT_TRACK_BUTTON = 13; // D13

  // ----------------------------------------------------------
  // NEW: WS2812B LED strip (64 LEDs) on A0 (data)
  // ----------------------------------------------------------
  // Treat A0 as a digital pin for FastLED.
  constexpr uint8_t  PIN_STRIP_DATA = A0; // A0
  constexpr uint16_t STRIP_NUM_LEDS = 64;

  // ----------------------------------------------------------
  // Dial LED behaviour constants (used by DisplayLED.*)
  // ----------------------------------------------------------
  // Folders 1–3: solid brightness
  // Folder 4: pulsing brightness synced to spooky theme
  constexpr uint8_t  DISPLAY_SOLID_BRIGHT  = 60;
  constexpr uint8_t  DISPLAY_PULSE_MIN     = 30;
  constexpr uint8_t  DISPLAY_PULSE_MAX     = 255;
  constexpr uint16_t DISPLAY_PULSE_TICK_MS = 18;
}

// ============================================================
// Compile-time pin conflict guards
// ============================================================
// These fail compilation if two subsystems are accidentally configured
// to use the same pin. This is a safety net for future edits.
//
// These are compile-time only and do not consume runtime SRAM.

// Helper macro to keep guards readable without changing behaviour.
#define CONFIG_ASSERT_PIN_DISTINCT(a, b, msg) static_assert((a) != (b), msg)
#define CONFIG_ASSERT_NOT_SERIAL_PINS(p, msg) static_assert(((p) != 0) && ((p) != 1), msg)

// ---- Basic internal pin sanity ----
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_BT_RX,  Config::PIN_BT_TX,  "Pin conflict: BT RX and BT TX must differ.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_MP3_RX, Config::PIN_MP3_TX, "Pin conflict: MP3 RX and MP3 TX must differ.");

// ---- Avoid using Serial pins where inappropriate ----
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_TUNING_INPUT,  "Invalid pin: tuning input must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_MATRIX_DATA,   "Invalid pin: matrix data must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_LED_DISPLAY,   "Invalid pin: dial LED must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_NEXT_TRACK_BUTTON, "Invalid pin: next-track button must not use D0/D1 (Serial).");
CONFIG_ASSERT_NOT_SERIAL_PINS(Config::PIN_STRIP_DATA,    "Invalid pin: strip data must not use D0/D1 (Serial).");

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
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_MATRIX_DATA,   "Pin conflict: strip data conflicts with matrix data.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_LED_DISPLAY,   "Pin conflict: strip data conflicts with dial LED.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_MP3_RX,        "Pin conflict: strip data conflicts with MP3 RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_MP3_TX,        "Pin conflict: strip data conflicts with MP3 TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_BT_RX,         "Pin conflict: strip data conflicts with BT RX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_BT_TX,         "Pin conflict: strip data conflicts with BT TX.");
CONFIG_ASSERT_PIN_DISTINCT(Config::PIN_STRIP_DATA, Config::PIN_SOURCE_DETECT, "Pin conflict: strip data conflicts with source detect.");

// Clean up helper macros to avoid leaking to other headers.
#undef CONFIG_ASSERT_PIN_DISTINCT
#undef CONFIG_ASSERT_NOT_SERIAL_PINS
