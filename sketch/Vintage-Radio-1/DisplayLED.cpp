
// DisplayLED.cpp
#include "DisplayLED.h"

namespace DisplayLED {
  static uint8_t  s_pin = 255;
  static uint8_t  s_currentBright = 0;
  static uint32_t s_lastTickMs = 0;

  void begin(uint8_t pin) {
    s_pin = pin;
    pinMode(s_pin, OUTPUT);
    s_currentBright = 0;
    analogWrite(s_pin, s_currentBright);
    s_lastTickMs = millis();
  }

  void setSolid(uint8_t brightness) {
    if (s_pin == 255) return;
    if (brightness != s_currentBright) {
      s_currentBright = brightness;
      analogWrite(s_pin, s_currentBright);
    }
  }

  void pulseSineTick(uint8_t bpm, uint8_t minBright, uint8_t maxBright, uint16_t tickMs) {
    if (s_pin == 255) return;
    const uint32_t now = millis();
    if (now - s_lastTickMs < tickMs) return;
    s_lastTickMs = now;

    const uint8_t pulse = beatsin8(bpm, minBright, maxBright);
    if (pulse != s_currentBright) {
      s_currentBright = pulse;
      analogWrite(s_pin, s_currentBright);
    }
  }

  void flickerRandomTick(uint8_t minBright, uint8_t maxBright, uint16_t tickMs) {
    if (s_pin == 255) return;
    const uint32_t now = millis();
    if (now - s_lastTickMs < tickMs) return;
    s_lastTickMs = now;

    if (maxBright < minBright) {
      const uint8_t tmp = minBright;
      minBright = maxBright;
      maxBright = tmp;
    }

    const uint8_t val = random8(minBright, (uint8_t)(maxBright + 1));
    if (val != s_currentBright) {
      s_currentBright = val;
      analogWrite(s_pin, s_currentBright);
    }
  }
}
