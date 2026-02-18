
// Radio_Tuning.cpp
#include "Radio_Tuning.h"
#include "Config.h"

// ---- Near real-time timing stream control ----
#ifndef DEBUG_TIMING_STREAM
 #define DEBUG_TIMING_STREAM 0
#endif
#ifndef DEBUG_TIMING_VERBOSE
 #define DEBUG_TIMING_VERBOSE 0
#endif

namespace {
  uint8_t RC_PIN = Config::PIN_TUNING_INPUT;

  const uint16_t DISCHARGE_MS = 10;
  const uint32_t TIMEOUT_US = 2000000UL; // 2 s safety

  // ---------------- Bands (µs) ----------------
  // These thresholds are unchanged; only the internal representation changes.
  const uint32_t F03_LOWER_US = 0;
  const uint32_t F03_UPPER_US = 30;

  const uint32_t F02_LOWER_US = 41;
  const uint32_t F02_UPPER_US = 51;

  const uint32_t F01_LOWER_US = 65;
  const uint32_t F01_UPPER_US = 89;

  const uint32_t F00_LOWER_US = 123; // >=123 → old "00" band

  // ---------------- Stability counts ----------------
  const uint8_t STABLE_COUNT_FOLDER = 4;
  const uint8_t STABLE_COUNT_GAP    = 4;

  // ---------------- State (numeric; avoids String heap use) ----------------
  // Standard return values:
  // 1..4 = folders, 99 = gap, 255 = fault
  static uint8_t currentFolder = 99;  // committed
  static uint8_t pendingClass  = 99;  // candidate classification
  static uint8_t pendingHits   = 0;

  // ---------- Measurement helpers ----------
  inline uint32_t measureRC_us_once() {
    pinMode(RC_PIN, OUTPUT);
    digitalWrite(RC_PIN, LOW);
    delay(DISCHARGE_MS);

    pinMode(RC_PIN, INPUT);

    const uint32_t t0 = micros();
    while (digitalRead(RC_PIN) == LOW) {
      if ((micros() - t0) > TIMEOUT_US) return 0; // fault/timeout
    }
    return micros() - t0;
  }

  // Median-of-three sampling (unchanged behaviour)
  inline uint32_t measureStable_us() {
    uint32_t a = measureRC_us_once(); if (a == 0) return 0;
    delay(2);
    uint32_t b = measureRC_us_once(); if (b == 0) return 0;
    delay(2);
    uint32_t c = measureRC_us_once(); if (c == 0) return 0;

  #if DEBUG_TIMING_VERBOSE == 1
    Serial.print("samples: a="); Serial.print(a);
    Serial.print(" b="); Serial.print(b);
    Serial.print(" c="); Serial.println(c);
  #endif

    // median-of-three
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
  }

  // Classify timing into standardized folder values (1..4, 99 gap)
  inline uint8_t classifyToFolder(uint32_t t_us) {
    // Gaps between bands -> 99
    if (t_us > F03_UPPER_US && t_us < F02_LOWER_US) return 99;
    if (t_us > F02_UPPER_US && t_us < F01_LOWER_US) return 99;
    if (t_us > F01_UPPER_US && t_us < F00_LOWER_US) return 99;

    // Band mapping (standardized):
    // old "03" -> folder 4
    // old "02" -> folder 3
    // old "01" -> folder 2
    // old "00" -> folder 1
    if (t_us >= F03_LOWER_US && t_us <= F03_UPPER_US) return 4;
    if (t_us >= F02_LOWER_US && t_us <= F02_UPPER_US) return 3;
    if (t_us >= F01_LOWER_US && t_us <= F01_UPPER_US) return 2;
    if (t_us >= F00_LOWER_US) return 1;

    return 99;
  }

  // Print helper: matches your “01..04 / 99 / FAULT” style
  inline void printFolderValue(uint8_t f) {
    if (f == 255) {
      Serial.print("FAULT");
      return;
    }
    if (f >= 1 && f <= 4) {
      if (f < 10) Serial.print('0');
      Serial.print(f);
      return;
    }
    Serial.print((int)f); // 99 (or anything unexpected)
  }

  inline void stepFolderSelect() {
    const uint32_t t_us = measureStable_us();

  #if DEBUG_TIMING_STREAM == 1
    const uint8_t inst = (t_us == 0) ? 255 : classifyToFolder(t_us);
    Serial.print("t_us="); Serial.print(t_us);
    Serial.print(" inst="); printFolderValue(inst);
    Serial.print(" committed="); printFolderValue(currentFolder);
    Serial.println();
  #endif

    // Fault path (timeout)
    if (t_us == 0) {
      if (currentFolder != 255) {
        currentFolder = 255;
        Serial.println("folder=FAULT");
      }
      pendingClass = 255;
      pendingHits = 0;
      return;
    }

    const uint8_t cls = classifyToFolder(t_us);

    if (cls == pendingClass) {
      pendingHits++;
    } else {
      pendingClass = cls;
      pendingHits = 1;
    }

    const bool isGap = (cls == 99);
    const uint8_t need = isGap ? STABLE_COUNT_GAP : STABLE_COUNT_FOLDER;

    if (cls != currentFolder && pendingHits >= need) {
      currentFolder = cls;

      // Preserve the existing style of “folder=...”
      Serial.print("folder=");
      if (currentFolder == 255) {
        Serial.println("FAULT");
      } else if (currentFolder >= 1 && currentFolder <= 4) {
        // print as 01..04
        if (currentFolder < 10) Serial.print('0');
        Serial.println(currentFolder);
      } else {
        Serial.println((int)currentFolder); // 99
      }

      Serial.print("t_us="); Serial.print(t_us);
      Serial.print(" cls=");  printFolderValue(cls);
      Serial.print(" hits="); Serial.println(pendingHits);
    }
  }
} // anonymous namespace

uint8_t RadioTuning::getFolder(uint8_t digitalPin) {
  RC_PIN = digitalPin;
  stepFolderSelect();
  return currentFolder;
}
