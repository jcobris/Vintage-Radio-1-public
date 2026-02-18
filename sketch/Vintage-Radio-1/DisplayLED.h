
// DisplayLED.h
#ifndef DISPLAYLED_H
#define DISPLAYLED_H

#include <Arduino.h>
#include <FastLED.h>  // for beatsin8 tempo utility

namespace DisplayLED {

  // Initialize PWM pin and set a default brightness
  void begin(uint8_t pin);

  // Set a solid brightness (0–255). Internally avoids redundant writes.
  void setSolid(uint8_t brightness);

  // Sine pulse between [minBright, maxBright] at a given BPM, throttled by tickMs.
  // Non-blocking; call every loop() tick.
  void pulseSineTick(uint8_t bpm, uint8_t minBright, uint8_t maxBright, uint16_t tickMs);
}

#endif // DISPLAYLED_H
