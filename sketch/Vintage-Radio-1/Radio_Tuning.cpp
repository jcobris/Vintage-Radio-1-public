
// Radio_Tuning.cpp
#include "Radio_Tuning.h"
#include "Config.h"

/*
 ============================================================
 Tuner mapping using RC timing (microseconds)
 ============================================================
 This module is designed to be:
 - Lightweight (Nano SRAM friendly)
 - Non-blocking beyond short, bounded measurement loops
 - Robust against single-sample noise via median-of-three

 Stability model:
 - Each call measures t_us and converts it to an instantaneous class:
   1..4, 99 (gap), or 255 (fault)
 - A change is "committed" only after N consecutive hits of the same class:
   STABLE_COUNT_FOLDER for folders 1..4
   STABLE_COUNT_GAP for gap 99
 - Committed values are returned by getFolder()

 Instantaneous class:
 - Stored in lastInstantClass and returned by getInstantClass()
 - Useful for "between stations" visuals without destabilizing audio
*/

namespace {
  uint8_t RC_PIN = Config::PIN_TUNING_INPUT;

  // Discharge time and safety timeout for measurement
  const uint16_t DISCHARGE_MS = 10;
  const uint32_t TIMEOUT_US = 2000000UL; // 2 seconds

  // ------------------------------------------------------------
  // Tuned thresholds (us)
  // ------------------------------------------------------------
  const uint32_t FOLDER4_LOWER_US = 0;
  const uint32_t FOLDER4_UPPER_US = 52;

  const uint32_t FOLDER3_LOWER_US = 60;
  const uint32_t FOLDER3_UPPER_US = 76;

  const uint32_t FOLDER2_LOWER_US = 96;
  const uint32_t FOLDER2_UPPER_US = 140;

  const uint32_t FOLDER1_LOWER_US = 172;

  // Stability requirements
  const uint8_t STABLE_COUNT_FOLDER = 4;
  const uint8_t STABLE_COUNT_GAP = 8;

  // Committed and pending state
  static uint8_t currentFolder = 99;
  static uint8_t pendingClass = 99;
  static uint8_t pendingHits = 0;

  // Instantaneous state
  static uint8_t lastInstantClass = 99;

  inline uint32_t measureRC_us_once() {
    // Discharge
    pinMode(RC_PIN, OUTPUT);
    digitalWrite(RC_PIN, LOW);
    delay(DISCHARGE_MS);

    // Measure charge time
    pinMode(RC_PIN, INPUT);
    const uint32_t t0 = micros();
    while (digitalRead(RC_PIN) == LOW) {
      if ((micros() - t0) > TIMEOUT_US) return 0;
    }
    return micros() - t0;
  }

  inline uint32_t measureStable_us() {
    uint32_t a = measureRC_us_once(); if (a == 0) return 0;
    delay(2);
    uint32_t b = measureRC_us_once(); if (b == 0) return 0;
    delay(2);
    uint32_t c = measureRC_us_once(); if (c == 0) return 0;

    if (TUNING_VERBOSE_DEBUG && DEBUG) {
      DBG_TUNING_VERBOSE2(F("samples: a="), a);
      DBG_TUNING_VERBOSE2(F(" b="), b);
      DBG_TUNING_VERBOSE2(F(" c="), c);
    }

    // Median-of-three
    if (((a <= b) && (b <= c)) || ((c <= b) && (b <= a))) return b;
    if (((b <= a) && (a <= c)) || ((c <= a) && (a <= b))) return a;
    return c;
  }

  inline uint8_t classifyToFolder(uint32_t t_us) {
    // gaps
    if (t_us > FOLDER4_UPPER_US && t_us < FOLDER3_LOWER_US) return 99;
    if (t_us > FOLDER3_UPPER_US && t_us < FOLDER2_LOWER_US) return 99;
    if (t_us > FOLDER2_UPPER_US && t_us < FOLDER1_LOWER_US) return 99;

    // bands
    if (t_us >= FOLDER4_LOWER_US && t_us <= FOLDER4_UPPER_US) return 4;
    if (t_us >= FOLDER3_LOWER_US && t_us <= FOLDER3_UPPER_US) return 3;
    if (t_us >= FOLDER2_LOWER_US && t_us <= FOLDER2_UPPER_US) return 2;
    if (t_us >= FOLDER1_LOWER_US) return 1;

    return 99;
  }

  inline void stepFolderSelect() {
    const uint32_t t_us = measureStable_us();

    // Fault/timeout
    if (t_us == 0) {
      lastInstantClass = 255;
      pendingClass = 255;
      pendingHits = 0;

      if (currentFolder != 255) {
        currentFolder = 255;
        DBG_TUNING(F("folder=FAULT"));
      }
      return;
    }

    const uint8_t cls = classifyToFolder(t_us);
    lastInstantClass = cls;

    // stream output (very noisy)
    if (TUNING_STREAM_DEBUG && DEBUG) {
      DBG_TUNING_STREAM2(F("t_us="), t_us);
      DBG_TUNING_STREAM2(F(" inst="), cls);
      DBG_TUNING_STREAM2(F(" committed="), currentFolder);
    }

    // stability tracking
    if (cls == pendingClass) pendingHits++;
    else { pendingClass = cls; pendingHits = 1; }

    const bool isGap = (cls == 99);
    const uint8_t need = isGap ? STABLE_COUNT_GAP : STABLE_COUNT_FOLDER;

    // commit only when stable enough
    if (cls != currentFolder && pendingHits >= need) {
      currentFolder = cls;

      if (currentFolder == 255) {
        DBG_TUNING(F("folder=FAULT"));
      } else {
        DBG_TUNING2(F("folder="), currentFolder);
      }

      if (TUNING_VERBOSE_DEBUG && DEBUG) {
        DBG_TUNING_VERBOSE2(F("t_us="), t_us);
        DBG_TUNING_VERBOSE2(F(" cls="), cls);
        DBG_TUNING_VERBOSE2(F(" hits="), pendingHits);
      }
    }
  }
}

uint8_t RadioTuning::getFolder(uint8_t digitalPin) {
  RC_PIN = digitalPin;
  stepFolderSelect();
  return currentFolder;
}

uint8_t RadioTuning::getInstantClass() {
  return lastInstantClass;
}
