
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

  // ============================================================
  // Standardised folder mapping (Option A):
  // Folder 4 = old "03" band
  // Folder 3 = old "02" band
  // Folder 2 = old "01" band
  // Folder 1 = old "00" band
  // 99 = gap / dead space between bands
  // 255 = fault (timeout)
  // ============================================================

  // Folder 4 (old "03")
  const uint32_t FOLDER4_LOWER_US = 0;
  const uint32_t FOLDER4_UPPER_US = 30; // your tuned value

  // Folder 3 (old "02")
  const uint32_t FOLDER3_LOWER_US = 36; // your tuned value
  const uint32_t FOLDER3_UPPER_US = 44; // your tuned value

  // Folder 2 (old "01")
  const uint32_t FOLDER2_LOWER_US = 52; // your tuned value
  const uint32_t FOLDER2_UPPER_US = 84; // your tuned value

  // Folder 1 (old "00")
  const uint32_t FOLDER1_LOWER_US = 100; // your tuned value

  // ---------------- Stability counts ----------------
  const uint8_t STABLE_COUNT_FOLDER = 4;
  const uint8_t STABLE_COUNT_GAP    = 8;  // your tuned value

  // ---------------- State (numeric; avoids String heap use) ----------------
  static uint8_t currentFolder = 99; // committed
  static uint8_t pendingClass  = 99; // candidate classification
  static uint8_t pendingHits   = 0;

  // NEW: latest instantaneous classification from last measurement (1 byte SRAM)
  static uint8_t lastInstantClass = 99;

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

  // Median-of-three sampling (unchanged behaviour) [1](https://teamtelstra-my.sharepoint.com/personal/jeff_c_cornwell_team_telstra_com/Documents/Microsoft%20Copilot%20Chat%20Files/Radio_Tuning.cpp)
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

  // Classify timing into standardized folder values (1..4, 99 gap) [1](https://teamtelstra-my.sharepoint.com/personal/jeff_c_cornwell_team_telstra_com/Documents/Microsoft%20Copilot%20Chat%20Files/Radio_Tuning.cpp)
  inline uint8_t classifyToFolder(uint32_t t_us) {
    // Gaps between bands -> 99
    if (t_us > FOLDER4_UPPER_US && t_us < FOLDER3_LOWER_US) return 99;
    if (t_us > FOLDER3_UPPER_US && t_us < FOLDER2_LOWER_US) return 99;
    if (t_us > FOLDER2_UPPER_US && t_us < FOLDER1_LOWER_US) return 99;

    // Band mapping (standardised folders)
    if (t_us >= FOLDER4_LOWER_US && t_us <= FOLDER4_UPPER_US) return 4;
    if (t_us >= FOLDER3_LOWER_US && t_us <= FOLDER3_UPPER_US) return 3;
    if (t_us >= FOLDER2_LOWER_US && t_us <= FOLDER2_UPPER_US) return 2;
    if (t_us >= FOLDER1_LOWER_US) return 1;

    return 99;
  }

  inline void printFolderValue(uint8_t f) {
    if (f == 255) { Serial.print("FAULT"); return; }
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
    // If t_us == 0 (timeout), inst is FAULT
    const uint8_t inst = (t_us == 0) ? 255 : classifyToFolder(t_us);
    Serial.print("t_us="); Serial.print(t_us);
    Serial.print(" inst="); printFolderValue(inst);
    Serial.print(" committed="); printFolderValue(currentFolder);
    Serial.println();
#endif

    // Fault path (timeout)
    if (t_us == 0) {
      lastInstantClass = 255; // NEW
      if (currentFolder != 255) {
        currentFolder = 255;
        Serial.println("folder=FAULT");
      }
      pendingClass = 255;
      pendingHits = 0;
      return;
    }

    const uint8_t cls = classifyToFolder(t_us);

    // NEW: record instantaneous classification every measurement
    lastInstantClass = cls;

    // Existing stability/commit logic (unchanged) [1](https://teamtelstra-my.sharepoint.com/personal/jeff_c_cornwell_team_telstra_com/Documents/Microsoft%20Copilot%20Chat%20Files/Radio_Tuning.cpp)
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
        if (currentFolder < 10) Serial.print('0');
        Serial.println(currentFolder);
      } else {
        Serial.println((int)currentFolder); // 99
      }

      Serial.print("t_us="); Serial.print(t_us);
      Serial.print(" cls="); printFolderValue(cls);
      Serial.print(" hits="); Serial.println(pendingHits);
    }
  }
} // anonymous namespace

uint8_t RadioTuning::getFolder(uint8_t digitalPin) {
  RC_PIN = digitalPin;
  stepFolderSelect();
  return currentFolder;
}

uint8_t RadioTuning::getInstantClass() {
  return lastInstantClass;
}
