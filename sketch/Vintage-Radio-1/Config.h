
// Config.h
#pragma once
#include <Arduino.h>

// ------------------------------------------------------------
// Global debug control (keep lightweight on Nano)
// ------------------------------------------------------------
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
// Project Pinout (SINGLE SOURCE OF TRUTH)
// Arduino Nano (ATmega328P)
// ------------------------------------------------------------
namespace Config {

  // --- Source detect ---
  // LOW -> MP3 selected
  // HIGH -> Bluetooth selected (INPUT_PULLUP)
  constexpr uint8_t PIN_SOURCE_DETECT = 2; // D2

  // --- 3-position Display Mode switch (3 poles to GND) ---
  // Use INPUT_PULLUP; "selected" = pin reads LOW (grounded).
  // D3 LOW: Normal display behaviour (as described)
  // D4 LOW: Alt theme (for now behaves same as normal)
  // D5 LOW: Matrix OFF, tuner LED string solid for all folders
  constexpr uint8_t PIN_DISPLAY_MODE_NORMAL = 3; // D3
  constexpr uint8_t PIN_DISPLAY_MODE_ALT    = 4; // D4
  constexpr uint8_t PIN_DISPLAY_MODE_OFF    = 5; // D5

  // --- Bluetooth module (BT201) UART via SoftwareSerial ---
  // BT201 TX -> Arduino RX pin
  // BT201 RX -> Arduino TX pin
  constexpr uint8_t PIN_BT_RX = 10; // D10 (Arduino RX for BT201 TX)
  constexpr uint8_t PIN_BT_TX = 9;  // D9  (Arduino TX for BT201 RX)
  constexpr long BT_BAUD = 57600;

  // --- MP3 module (DY-SV5W) UART via SoftwareSerial ---
  // DY-SV5W TXD -> Arduino RX pin
  // DY-SV5W RXD -> Arduino TX pin
  constexpr uint8_t PIN_MP3_RX = 12; // D12 (Arduino RX for MP3 TXD)
  constexpr uint8_t PIN_MP3_TX = 11; // D11 (Arduino TX for MP3 RXD)
  constexpr long MP3_BAUD = 9600;

  // --- Tuning input (RC timing input) ---
  // IMPORTANT: Do NOT use D0/D1 because Serial uses them.
  constexpr uint8_t PIN_TUNING_INPUT = 8; // D8

  // --- LED string (tuning illumination) ---
  // Requirement: LED string = D6
  constexpr uint8_t PIN_LED_DISPLAY = 6; // D6 (PWM-capable)

  // --- LED Matrix data pin ---
  // Requirement: Matrix data = D7
  constexpr uint8_t PIN_MATRIX_DATA = 7; // D7

  // ----------------------------------------------------------
  // Display behaviour constants (single source of truth)
  // ----------------------------------------------------------
  constexpr uint8_t  DISPLAY_SOLID_BRIGHT = 60;  // folders 1–3 solid
  constexpr uint8_t  DISPLAY_PULSE_MIN    = 30;  // folder 4 pulse min
  constexpr uint8_t  DISPLAY_PULSE_MAX    = 255; // folder 4 pulse max
  constexpr uint16_t DISPLAY_PULSE_TICK_MS = 18; // folder 4 pulse cadence
}

// ------------------------------------------------------------
// Compile-time pin conflict guards (fail fast if pins collide)
// ------------------------------------------------------------
static_assert(Config::PIN_BT_RX != Config::PIN_BT_TX, "Pin conflict: BT RX and BT TX must differ.");
static_assert(Config::PIN_MP3_RX != Config::PIN_MP3_TX, "Pin conflict: MP3 RX and MP3 TX must differ.");

static_assert(Config::PIN_LED_DISPLAY != Config::PIN_MP3_TX, "Pin conflict: LED display pin conflicts with MP3 TX.");
static_assert(Config::PIN_LED_DISPLAY != Config::PIN_MP3_RX, "Pin conflict: LED display pin conflicts with MP3 RX.");
static_assert(Config::PIN_LED_DISPLAY != Config::PIN_BT_TX,  "Pin conflict: LED display pin conflicts with BT TX.");
static_assert(Config::PIN_LED_DISPLAY != Config::PIN_BT_RX,  "Pin conflict: LED display pin conflicts with BT RX.");

static_assert(Config::PIN_MATRIX_DATA != Config::PIN_MP3_TX, "Pin conflict: Matrix data pin conflicts with MP3 TX.");
static_assert(Config::PIN_MATRIX_DATA != Config::PIN_MP3_RX, "Pin conflict: Matrix data pin conflicts with MP3 RX.");
static_assert(Config::PIN_MATRIX_DATA != Config::PIN_BT_TX,  "Pin conflict: Matrix data pin conflicts with BT TX.");
static_assert(Config::PIN_MATRIX_DATA != Config::PIN_BT_RX,  "Pin conflict: Matrix data pin conflicts with BT RX.");

static_assert(Config::PIN_MATRIX_DATA != Config::PIN_LED_DISPLAY, "Pin conflict: Matrix data and LED display pins must differ.");

static_assert(Config::PIN_TUNING_INPUT != 0 && Config::PIN_TUNING_INPUT != 1, "Invalid pin: tuning input must not use D0/D1 (Serial).");
static_assert(Config::PIN_MATRIX_DATA  != 0 && Config::PIN_MATRIX_DATA  != 1, "Invalid pin: matrix data must not use D0/D1 (Serial).");
static_assert(Config::PIN_LED_DISPLAY  != 0 && Config::PIN_LED_DISPLAY  != 1, "Invalid pin: LED display must not use D0/D1 (Serial).");

// Display mode switch pins should not collide with existing assignments
static_assert(Config::PIN_DISPLAY_MODE_NORMAL != Config::PIN_BT_RX, "Pin conflict: display switch vs BT RX.");
static_assert(Config::PIN_DISPLAY_MODE_NORMAL != Config::PIN_BT_TX, "Pin conflict: display switch vs BT TX.");
static_assert(Config::PIN_DISPLAY_MODE_NORMAL != Config::PIN_MP3_RX, "Pin conflict: display switch vs MP3 RX.");
static_assert(Config::PIN_DISPLAY_MODE_NORMAL != Config::PIN_MP3_TX, "Pin conflict: display switch vs MP3 TX.");
static_assert(Config::PIN_DISPLAY_MODE_NORMAL != Config::PIN_LED_DISPLAY, "Pin conflict: display switch vs LED display.");
static_assert(Config::PIN_DISPLAY_MODE_NORMAL != Config::PIN_MATRIX_DATA, "Pin conflict: display switch vs matrix data.");
static_assert(Config::PIN_DISPLAY_MODE_NORMAL != Config::PIN_SOURCE_DETECT, "Pin conflict: display switch vs source detect.");

static_assert(Config::PIN_DISPLAY_MODE_ALT != Config::PIN_BT_RX, "Pin conflict: display switch vs BT RX.");
static_assert(Config::PIN_DISPLAY_MODE_ALT != Config::PIN_BT_TX, "Pin conflict: display switch vs BT TX.");
static_assert(Config::PIN_DISPLAY_MODE_ALT != Config::PIN_MP3_RX, "Pin conflict: display switch vs MP3 RX.");
static_assert(Config::PIN_DISPLAY_MODE_ALT != Config::PIN_MP3_TX, "Pin conflict: display switch vs MP3 TX.");
static_assert(Config::PIN_DISPLAY_MODE_ALT != Config::PIN_LED_DISPLAY, "Pin conflict: display switch vs LED display.");
static_assert(Config::PIN_DISPLAY_MODE_ALT != Config::PIN_MATRIX_DATA, "Pin conflict: display switch vs matrix data.");
static_assert(Config::PIN_DISPLAY_MODE_ALT != Config::PIN_SOURCE_DETECT, "Pin conflict: display switch vs source detect.");

static_assert(Config::PIN_DISPLAY_MODE_OFF != Config::PIN_BT_RX, "Pin conflict: display switch vs BT RX.");
static_assert(Config::PIN_DISPLAY_MODE_OFF != Config::PIN_BT_TX, "Pin conflict: display switch vs BT TX.");
static_assert(Config::PIN_DISPLAY_MODE_OFF != Config::PIN_MP3_RX, "Pin conflict: display switch vs MP3 RX.");
static_assert(Config::PIN_DISPLAY_MODE_OFF != Config::PIN_MP3_TX, "Pin conflict: display switch vs MP3 TX.");
static_assert(Config::PIN_DISPLAY_MODE_OFF != Config::PIN_LED_DISPLAY, "Pin conflict: display switch vs LED display.");
static_assert(Config::PIN_DISPLAY_MODE_OFF != Config::PIN_MATRIX_DATA, "Pin conflict: display switch vs matrix data.");
static_assert(Config::PIN_DISPLAY_MODE_OFF != Config::PIN_SOURCE_DETECT, "Pin conflict: display switch vs source detect.");
