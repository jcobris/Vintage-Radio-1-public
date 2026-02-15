
// Config.h
#pragma once
#include <Arduino.h>

// ---- Global debug control ----
#ifndef DEBUG
  #define DEBUG 1
#endif

#if DEBUG == 1
  #define debug(x)     Serial.print(x)
  #define debugln(x)   Serial.println(x)
#else
  #define debug(x)     do {} while (0)
  #define debugln(x)   do {} while (0)
#endif

namespace Config {
  // --- Pins ---
  constexpr uint8_t PIN_TUNING_INPUT = 6;    // D6 for RC timing
  constexpr uint8_t PIN_LED_DISPLAY  = 11;   // D11 to MOSFET/LED string
}
