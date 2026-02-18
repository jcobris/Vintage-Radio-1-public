
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
  const uint32_t TIMEOUT_US = 2000000; // 2 s safety

  // ---------------- Bands (µs) ----------------
  const uint32_t F03_LOWER_US = 0;
  const uint32_t F03_UPPER_US = 30;
  const uint32_t F02_LOWER_US = 41;
  const uint32_t F02_UPPER_US = 51;
  const uint32_t F01_LOWER_US = 65;
  const uint32_t F01_UPPER_US = 89;
  const uint32_t F00_LOWER_US = 123; // >=123 → folder 00 (no upper bound)

  // ---------------- Stability counts ----------------
  const uint8_t STABLE_COUNT_FOLDER = 4;
  const uint8_t STABLE_COUNT_GAP = 4;

  // ---------------- State ----------------
  String currentFolder = "99"; // committed ("00".."03","99","FAULT")
  String pendingClass = "99";
  uint8_t pendingHits = 0;

  inline uint32_t measureRC_us_once() {
    pinMode(RC_PIN, OUTPUT);
    digitalWrite(RC_PIN, LOW);
    delay(DISCHARGE_MS);

    pinMode(RC_PIN, INPUT);
    uint32_t t0 = micros();
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

  #if DEBUG_TIMING_VERBOSE == 1
    Serial.print("samples: a="); Serial.print(a);
    Serial.print(" b="); Serial.print(b);
    Serial.print(" c="); Serial.println(c);
  #endif

    // Cleaned: explicit boolean OR (median-of-three)
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
  }

  inline String classifyBuckets(uint32_t t) {
    if (t > F03_UPPER_US && t < F02_LOWER_US) return "99";
    if (t > F02_UPPER_US && t < F01_LOWER_US) return "99";
    if (t > F01_UPPER_US && t < F00_LOWER_US) return "99";

    if (t >= F03_LOWER_US && t <= F03_UPPER_US) return "03";
    if (t >= F02_LOWER_US && t <= F02_UPPER_US) return "02";
    if (t >= F01_LOWER_US && t <= F01_UPPER_US) return "01";
    if (t >= F00_LOWER_US) return "00";

    return "99";
  }

  // CHANGED: map "00..03" to 1..4
  inline uint8_t classToNumeric(const String& cls) {
    if (cls == "00") return 1;
    if (cls == "01") return 2;
    if (cls == "02") return 3;
    if (cls == "03") return 4;
    if (cls == "99") return 99;
    if (cls == "FAULT") return 255;
    return 99;
  }

  inline void printFolderColumn(const String& clsOrCommitted) {
    if (clsOrCommitted == "FAULT") {
      Serial.print("FAULT");
      return;
    }
    uint8_t num = classToNumeric(clsOrCommitted);
    if (num >= 1 && num <= 4) {
      // Print as 01..04 to mirror old formatting
      if (num < 10) Serial.print('0');
      Serial.print(num);
    } else {
      Serial.print(num); // 99
    }
  }

  inline void stepFolderSelect() {
    uint32_t t_us = measureStable_us();

  #if DEBUG_TIMING_STREAM == 1
    String clsNow = (t_us == 0) ? "FAULT" : classifyBuckets(t_us);
    Serial.print("t_us="); Serial.print(t_us);
    Serial.print(" inst=");
    printFolderColumn(clsNow);
    Serial.print(" committed=");
    printFolderColumn(currentFolder);
    Serial.println();
  #endif

    if (t_us == 0) {
      if (currentFolder != "FAULT") {
        currentFolder = "FAULT";
        Serial.println("folder=FAULT");
      }
      pendingClass = "FAULT";
      pendingHits = 0;
      return;
    }

    String cls = classifyBuckets(t_us);

    if (cls == pendingClass) {
      pendingHits++;
    } else {
      pendingClass = cls;
      pendingHits = 1;
    }

    const bool isGap = (cls == "99");
    const uint8_t need = isGap ? STABLE_COUNT_GAP : STABLE_COUNT_FOLDER;

    if (cls != currentFolder && pendingHits >= need) {
      currentFolder = cls;
      Serial.print("folder="); Serial.println(currentFolder);
      Serial.print("t_us="); Serial.print(t_us);
      Serial.print(" cls="); Serial.print(cls);
      Serial.print(" hits="); Serial.println(pendingHits);
    }
  }

  inline uint8_t toNumericFolder(const String& s) {
    return classToNumeric(s);
  }
} // anonymous namespace

uint8_t RadioTuning::getFolder(uint8_t digitalPin) {
  RC_PIN = digitalPin;
  stepFolderSelect();
  return toNumericFolder(currentFolder);
}
