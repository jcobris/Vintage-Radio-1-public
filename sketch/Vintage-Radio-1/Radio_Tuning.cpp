
// Radio_Tuning.cpp
#include "Radio_Tuning.h"
#include "Config.h"

// Optional timing stream controls (compile-time).
// Leave these as 0 for normal operation.
#ifndef DEBUG_TIMING_STREAM
  #define DEBUG_TIMING_STREAM 0
#endif
#ifndef DEBUG_TIMING_VERBOSE
  #define DEBUG_TIMING_VERBOSE 0
#endif

namespace {
  uint8_t RC_PIN = Config::PIN_TUNING_INPUT;

  // Discharge and safety timeout for the RC timing measurement.
  const uint16_t DISCHARGE_MS = 10;
  const uint32_t TIMEOUT_US   = 2000000UL; // 2 seconds

  // ------------------------------------------------------------------
  // Standard folder mapping (Option A)
  // ------------------------------------------------------------------
  // Folder 4 = "Band 4" (old 03)
  // Folder 3 = "Band 3" (old 02)
  // Folder 2 = "Band 2" (old 01)
  // Folder 1 = "Band 1" (old 00)
  //
  // Returns:
  //   1..4 = stable folder
  //   99   = gap / between bands
  //   255  = fault (timeout)
  // ------------------------------------------------------------------

  // Folder 4 (your tuned values)
  const uint32_t FOLDER4_LOWER_US = 0;
  const uint32_t FOLDER4_UPPER_US = 30;

  // Folder 3
  const uint32_t FOLDER3_LOWER_US = 36;
  const uint32_t FOLDER3_UPPER_US = 44;

  // Folder 2
  const uint32_t FOLDER2_LOWER_US = 60;
  const uint32_t FOLDER2_UPPER_US = 84;

  // Folder 1 (anything >= this)
  const uint32_t FOLDER1_LOWER_US = 100;

  // Stability thresholds for committing a change.
  // Folders commit quickly; gaps require more persistence.
  const uint8_t STABLE_COUNT_FOLDER = 4;
  const uint8_t STABLE_COUNT_GAP    = 8;

  // Committed and pending state.
  static uint8_t currentFolder = 99;
  static uint8_t pendingClass  = 99;
  static uint8_t pendingHits   = 0;

  // Instantaneous classification from the most recent measurement.
  static uint8_t lastInstantClass = 99;

  inline uint32_t measureRC_us_once() {
    // Discharge capacitor
    pinMode(RC_PIN, OUTPUT);
    digitalWrite(RC_PIN, LOW);
    delay(DISCHARGE_MS);

    // Measure charge time until digital threshold is crossed
    pinMode(RC_PIN, INPUT);
    const uint32_t t0 = micros();

    while (digitalRead(RC_PIN) == LOW) {
      if ((micros() - t0) > TIMEOUT_US) return 0; // timeout -> fault
    }
    return micros() - t0;
  }

  inline uint32_t measureStable_us() {
    // Median-of-three to reduce single-sample noise
    uint32_t a = measureRC_us_once(); if (a == 0) return 0;
    delay(2);
    uint32_t b = measureRC_us_once(); if (b == 0) return 0;
    delay(2);
    uint32_t c = measureRC_us_once(); if (c == 0) return 0;

#if DEBUG_TIMING_VERBOSE == 1
    if (DEBUG == 1) {
      Serial.print(F("samples: a=")); Serial.print(a);
      Serial.print(F(" b="));         Serial.print(b);
      Serial.print(F(" c="));         Serial.println(c);
    }
#endif

    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
  }

  inline uint8_t classifyToFolder(uint32_t t_us) {
    // Gaps between bands -> 99
    if (t_us > FOLDER4_UPPER_US && t_us < FOLDER3_LOWER_US) return 99;
    if (t_us > FOLDER3_UPPER_US && t_us < FOLDER2_LOWER_US) return 99;
    if (t_us > FOLDER2_UPPER_US && t_us < FOLDER1_LOWER_US) return 99;

    // Bands -> folders
    if (t_us >= FOLDER4_LOWER_US && t_us <= FOLDER4_UPPER_US) return 4;
    if (t_us >= FOLDER3_LOWER_US && t_us <= FOLDER3_UPPER_US) return 3;
    if (t_us >= FOLDER2_LOWER_US && t_us <= FOLDER2_UPPER_US) return 2;
    if (t_us >= FOLDER1_LOWER_US) return 1;

    return 99;
  }

  inline void stepFolderSelect() {
    const uint32_t t_us = measureStable_us();

    // Fault path
    if (t_us == 0) {
      lastInstantClass = 255;
      pendingClass = 255;
      pendingHits = 0;

      if (currentFolder != 255) {
        currentFolder = 255;
        if (DEBUG == 1) Serial.println(F("folder=FAULT"));
      }
      return;
    }

    const uint8_t cls = classifyToFolder(t_us);
    lastInstantClass = cls;

#if DEBUG_TIMING_STREAM == 1
    if (DEBUG == 1) {
      Serial.print(F("t_us="));      Serial.print(t_us);
      Serial.print(F(" inst="));     Serial.print(cls);
      Serial.print(F(" committed="));Serial.println(currentFolder);
    }
#endif

    if (cls == pendingClass) pendingHits++;
    else { pendingClass = cls; pendingHits = 1; }

    const bool isGap = (cls == 99);
    const uint8_t need = isGap ? STABLE_COUNT_GAP : STABLE_COUNT_FOLDER;

    if (cls != currentFolder && pendingHits >= need) {
      currentFolder = cls;

      // Only print commit messages when DEBUG is enabled.
      if (DEBUG == 1) {
        Serial.print(F("folder="));
        Serial.println(currentFolder);
        Serial.print(F("t_us=")); Serial.print(t_us);
        Serial.print(F(" cls=")); Serial.print(cls);
        Serial.print(F(" hits="));Serial.println(pendingHits);
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
