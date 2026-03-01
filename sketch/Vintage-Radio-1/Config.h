
// Config.h
#pragma once
#include <Arduino.h>

/*
  ============================================================
  Project Configuration (Single Source of Truth)
  ============================================================

  This header defines:
  - Pin assignments (Arduino Nano / ATmega328P)
  - Baud rates for the two SoftwareSerial UARTs (BT + MP3)
  - Dial LED (tuning illumination) brightness parameters
  - Compile-time debug gates (Serial prints)

  Notes on resource constraints:
  - The Nano has 2 KB SRAM. Avoid dynamic allocation and large buffers.
  - DEBUG is OFF by default to reduce Serial overhead and timing jitter.
*/

// ------------------------------------------------------------
// Global debug control
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Pinout (Arduino Nano / ATmega328P)
// ------------------------------------------------------------
namespace Config {

  // ----------------------------------------------------------
  // Source detect input (physical source select switch)
  // ----------------------------------------------------------
  // The audio path is switched physically by the radio’s hardware.
  // The Arduino only *detects* which source is active to drive displays / MP3 control.
  //
  // Pin mode: INPUT_PULLUP
  // LOW  -> MP3 selected
  // HIGH -> Bluetooth selected
  constexpr uint8_t PIN_SOURCE_DETECT = 2; // D2

  // ----------------------------------------------------------
  // 3-position display mode switch (3 poles to GND)
  // ----------------------------------------------------------
  // Wiring: each pole grounds one pin when selected.
  // Pin mode: INPUT_PULLUP
  // LOW = selected
  //
  // NORMAL:  Normal theme behaviour
  // ALT:     Reserved; currently same as normal
  // OFF:     Matrix OFF; dial LED forced solid ON
  constexpr uint8_t PIN_DISPLAY_MODE_NORMAL = 3; // D3
  constexpr uint8_t PIN_DISPLAY_MODE_ALT    = 4; // D4
  constexpr uint8_t PIN_DISPLAY_MODE_OFF    = 5; // D5

  // ----------------------------------------------------------
  // Bluetooth module (BT201) UART (SoftwareSerial)
  // ----------------------------------------------------------
  // RX/TX naming is from Arduino’s perspective:
  // - PIN_BT_RX is the Arduino RX pin (connects to BT TX)
  // - PIN_BT_TX is the Arduino TX pin (connects to BT RX)
  //
  // The Bluetooth UART is typically slept to avoid SoftwareSerial contention.
  constexpr uint8_t PIN_BT_RX = 10; // D10 (Arduino RX for BT TX)
  constexpr uint8_t PIN_BT_TX = 9;  // D9  (Arduino TX for BT RX)
  constexpr long    BT_BAUD   = 57600;

  // ----------------------------------------------------------
  // MP3 module (DY-SV5W) UART (SoftwareSerial)
  // ----------------------------------------------------------
  // RX/TX naming is from Arduino’s perspective:
  // - PIN_MP3_RX is the Arduino RX pin (connects to MP3 TXD)
  // - PIN_MP3_TX is the Arduino TX pin (connects to MP3 RXD)
  //
  // MP3 runs continuously while MP3 is the selected source.
  constexpr uint8_t PIN_MP3_RX = 12; // D12 (Arduino RX for MP3 TXD)
  constexpr uint8_t PIN_MP3_TX = 11; // D11 (Arduino TX for MP3 RXD)
  constexpr long    MP3_BAUD   = 9600;

  // ----------------------------------------------------------
  // Tuning input (RC timing input)
  // ----------------------------------------------------------
  // This pin is connected to an RC network driven by the tuning mechanism.
  // The firmware measures a charge-time in microseconds and maps it to folders.
  //
  // IMPORTANT: Avoid D0/D1 because those pins are used by hardware Serial.
  constexpr uint8_t PIN_TUNING_INPUT = 8; // D8

  // ----------------------------------------------------------
  // Dial / tuner illumination LED string (PWM)
  // ----------------------------------------------------------
  // Uses analogWrite() on a PWM-capable pin.
  constexpr uint8_t PIN_LED_DISPLAY = 6; // D6

  // ----------------------------------------------------------
  // WS2812B 8x32 LED matrix data pin
  // ----------------------------------------------------------
  constexpr uint8_t PIN_MATRIX_DATA = 7; // D7

  // ----------------------------------------------------------
  // Dial LED behaviour constants
  // ----------------------------------------------------------
  // These are used by DisplayLED.*
  // Folders 1–3: solid brightness
  // Folder 4: pulsing brightness synced to spooky theme
  constexpr uint8_t  DISPLAY_SOLID_BRIGHT  = 60;
  constexpr uint8_t  DISPLAY_PULSE_MIN     = 30;
  constexpr uint8_t  DISPLAY_PULSE_MAX     = 255;
  constexpr uint16_t DISPLAY_PULSE_TICK_MS = 18;
}

// ------------------------------------------------------------
// Compile-time pin conflict guards
// ------------------------------------------------------------
// These fail compilation if two subsystems are accidentally configured to use
// the same pin. This is a safety net for future edits.
static_assert(Config::PIN_BT_RX != Config::PIN_BT_TX, "Pin conflict: BT RX and BT TX must differ.");
static_assert(Config::PIN_MP3_RX != Config::PIN_MP3_TX, "Pin conflict: MP3 RX and MP3 TX must differ.");

static_assert(Config::PIN_LED_DISPLAY != Config::PIN_MP3_TX, "Pin conflict: dial LED pin conflicts with MP3 TX.");
static_assert(Config::PIN_LED_DISPLAY != Config::PIN_MP3_RX, "Pin conflict: dial LED pin conflicts with MP3 RX.");
static_assert(Config::PIN_LED_DISPLAY != Config::PIN_BT_TX,  "Pin conflict: dial LED pin conflicts with BT TX.");
static_assert(Config::PIN_LED_DISPLAY != Config::PIN_BT_RX,  "Pin conflict: dial LED pin conflicts with BT RX.");

static_assert(Config::PIN_MATRIX_DATA != Config::PIN_MP3_TX, "Pin conflict: matrix pin conflicts with MP3 TX.");
static_assert(Config::PIN_MATRIX_DATA != Config::PIN_MP3_RX, "Pin conflict: matrix pin conflicts with MP3 RX.");
static_assert(Config::PIN_MATRIX_DATA != Config::PIN_BT_TX,  "Pin conflict: matrix pin conflicts with BT TX.");
static_assert(Config::PIN_MATRIX_DATA != Config::PIN_BT_RX,  "Pin conflict: matrix pin conflicts with BT RX.");
static_assert(Config::PIN_MATRIX_DATA != Config::PIN_LED_DISPLAY, "Pin conflict: matrix and dial LED pins must differ.");

static_assert(Config::PIN_TUNING_INPUT != 0 && Config::PIN_TUNING_INPUT != 1,
              "Invalid pin: tuning input must not use D0/D1 (Serial).");
static_assert(Config::PIN_MATRIX_DATA != 0 && Config::PIN_MATRIX_DATA != 1,
              "Invalid pin: matrix data must not use D0/D1 (Serial).");
static_assert(Config::PIN_LED_DISPLAY != 0 && Config::PIN_LED_DISPLAY != 1,
              "Invalid pin: dial LED must not use D0/D1 (Serial).");
