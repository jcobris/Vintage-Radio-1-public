
// DisplayLED.h
#ifndef DISPLAYLED_H
#define DISPLAYLED_H

#include <Arduino.h>
#include <FastLED.h>

/*
  ============================================================
  Dial / Tuning Illumination LED (PWM)
  ============================================================

  The "dial LED" is driven by analogWrite() (PWM) and supports:
  - Solid brightness
  - Slow sine pulse (used for Folder 4 spooky theme)
  - Quick random flicker (used for between-stations visual static)

  Implementation notes:
  - Avoids redundant PWM writes (only writes when value changes)
  - Tick-based functions are throttled to reduce CPU overhead
  - Uses minimal static state to remain Nano-friendly
*/

namespace DisplayLED {

  // Initialize the PWM pin and set brightness to 0.
  void begin(uint8_t pin);

  // Set a constant brightness level (0..255).
  // Internally avoids redundant analogWrite calls.
  void setSolid(uint8_t brightness);

  // Pulse brightness between minBright and maxBright using a sine wave at bpm.
  // Call frequently; the function will self-throttle to tickMs.
  void pulseSineTick(uint8_t bpm, uint8_t minBright, uint8_t maxBright, uint16_t tickMs);

  // Random flicker brightness between minBright and maxBright.
  // Useful for "between stations" static effect.
  // Call frequently; the function will self-throttle to tickMs.
  void flickerRandomTick(uint8_t minBright, uint8_t maxBright, uint16_t tickMs);
}

#endif // DISPLAYLED_H
