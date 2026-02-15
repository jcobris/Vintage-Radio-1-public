original display_LED.cpp


// Display_LED.cpp
#include "Display_LED.h"

// Private module state
namespace {
  uint8_t s_displayPin = 255;     // invalid until init() sets it
  int     s_brightness = 250;     // starting brightness (0..255)
  int     s_fadeStep   = 5;       // PWM increment per update
  unsigned long s_lastMs = 0;     // for non-blocking timing
  const uint16_t s_stepIntervalMs = 35; // update cadence

  inline bool ready() { return s_displayPin != 255; }

  inline void writePWM(int level) {
    // Clamp to 0..255 before writing
    int clamped = (level < 0) ? 0 : (level > 255 ? 255 : level);
    analogWrite(s_displayPin, clamped);
  }
}

void DisplayLED::init(uint8_t displayPin) {
  s_displayPin = displayPin;
  pinMode(s_displayPin, OUTPUT);
  // Set initial state to solid (in case folder != 0 at start)
  writePWM(s_brightness);
  debugln("DisplayLED: init complete");
}

void DisplayLED::update(uint8_t folder) {
  if (!ready()) return;

  // Non-blocking timing
  unsigned long now = millis();
  if (now - s_lastMs < s_stepIntervalMs) {
    return; // too soon, skip this frame
  }
  s_lastMs = now;

  if (folder == 0) {
    // Pulsate: adjust brightness, then write it
    s_brightness += s_fadeStep;

    // Reverse at edges (soft floor to avoid near-off)
    if (s_brightness <= 30 || s_brightness >= 255) {
      s_fadeStep = -s_fadeStep;
      // Keep within bounds after flip
      if (s_brightness < 30)  s_brightness = 30;
      if (s_brightness > 255) s_brightness = 255;
    }

    writePWM(s_brightness);

    // Optional debug
    debug("Display LED (pulse) brightness: ");
    debugln(s_brightness);
  } else {
    // Solid brightness: fixed steady PWM level
    const int solidLevel = 255;
    s_brightness = solidLevel;
    writePWM(solidLevel);

    // Optional debug
    debug("Display LED (solid) brightness: ");
    debugln(solidLevel);
  }
}
