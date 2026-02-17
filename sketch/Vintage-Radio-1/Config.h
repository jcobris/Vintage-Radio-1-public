
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
  // LOW  -> MP3 selected
  // HIGH -> Bluetooth selected (INPUT_PULLUP)
  constexpr uint8_t PIN_SOURCE_DETECT = 2;   // D2

  // --- Bluetooth module (BT201) UART via SoftwareSerial ---
  // BT201 TX -> Arduino RX pin
  // BT201 RX -> Arduino TX pin
  constexpr uint8_t PIN_BT_RX = 10;          // D10 (Arduino RX for BT201 TX)
  constexpr uint8_t PIN_BT_TX = 9;           // D9  (Arduino TX for BT201 RX)
  constexpr long    BT_BAUD   = 57600;

  // --- MP3 module (DY-SV5W) UART via SoftwareSerial ---
  // DY-SV5W TXD -> Arduino RX pin
  // DY-SV5W RXD -> Arduino TX pin
  constexpr uint8_t PIN_MP3_RX = 12;         // D12 (Arduino RX for MP3 TXD)
  constexpr uint8_t PIN_MP3_TX = 11;         // D11 (Arduino TX for MP3 RXD)
  constexpr long    MP3_BAUD   = 9600;

  // --- Tuning input (RC timing input) ---
  // IMPORTANT: Do NOT use D0/D1 because Serial uses them.
  constexpr uint8_t PIN_TUNING_INPUT = 8;    // D8 (spare digital pin)

  // --- LED display output (tuner dial / LED string) ---
  // Using D6 to avoid conflict with MP3 TX (D11).
  constexpr uint8_t PIN_LED_DISPLAY = 6;     // D6 (PWM-capable)

  // --- LED Matrix data pin (WS2812 / etc.) ---
  // Not used on breadboard currently, but reserved here.
  constexpr uint8_t PIN_MATRIX_DATA = 7;     // D7
}
