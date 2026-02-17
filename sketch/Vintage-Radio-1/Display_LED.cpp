
// Display_LED.cpp
#include "Display_LED.h"

// ---- Near real-time display stream control ----
// Set to 1 to print brightness on every pulse tick (continuous while pulsing).
// Set to 0 to silence the stream.
#ifndef DEBUG_DISPLAY_STREAM
  #define DEBUG_DISPLAY_STREAM 0
#endif

// Set to 1 to also print extra details per tick (noisy; good for tuning).
#ifndef DEBUG_DISPLAY_VERBOSE
  #define DEBUG_DISPLAY_VERBOSE 0
#endif

namespace {
  uint8_t       s_displayPin = 11;     // updated in init()
  unsigned long s_lastMs     = 0;

  // Pulse state
  int           s_brightness = 60;     // starting brightness
  int           s_fadeStep   = 3;      // set to 5 if you want faster ramp
  const int     BRIGHT_MIN   = 30;
  const int     BRIGHT_MAX   = 255;

  inline bool ready() { return s_displayPin != 255; }

  inline void printStream(uint16_t intervalMs) {
    #if DEBUG_DISPLAY_STREAM == 1
      // Compact one-line stream for easy scanning
      Serial.print("led_brightness=");
      Serial.print(s_brightness);
      Serial.print(" pin=");
      Serial.print(s_displayPin);
      Serial.print(" interval=");
      Serial.println(intervalMs);

      // Optional verbose details
      #if DEBUG_DISPLAY_VERBOSE == 1
        Serial.print("step=");
        Serial.print(s_fadeStep);
        Serial.print(" bounds=[");
        Serial.print(BRIGHT_MIN);
        Serial.print("..");
        Serial.print(BRIGHT_MAX);
        Serial.println("]");
      #endif
    #endif
  }
}

void DisplayLED::init(uint8_t displayPin) {
  s_displayPin = displayPin;
  pinMode(s_displayPin, OUTPUT);

  // Default to digital HIGH (full brightness, no PWM interference)
  digitalWrite(s_displayPin, HIGH);

  // Optional single-line init confirmation (kept off by default for quiet logs)
  #if DEBUG_DISPLAY_VERBOSE == 1
    Serial.print("DisplayLED: init pin=");
    Serial.println(s_displayPin);
  #endif
}

void DisplayLED::pulseTick(uint16_t intervalMs) {
  if (!ready()) return;

  const unsigned long now = millis();
  if ((now - s_lastMs) < intervalMs) return;
  s_lastMs = now;

  // Drive PWM for the pulse
  analogWrite(s_displayPin, s_brightness);

  // Update brightness (linear ramp)
  s_brightness += s_fadeStep;

  // Reverse at edges (stick to tested bounds)
  if (s_brightness <= BRIGHT_MIN || s_brightness >= BRIGHT_MAX) {
    s_fadeStep = -s_fadeStep;
  }

  // Defensive clamp
  s_brightness = constrain(s_brightness, 0, 255);

  // Stream (Radio_Tuning-style)
  printStream(intervalMs);
}
