
// Radio_Tuning.cpp
#include "Radio_Tuning.h"
#include "Config.h"   // for pins + (optional) debug macros; ensure it's in your project

// ---- Near real-time timing stream control ----
// Set to 1 to print t_us for every measurement step (continuous while sweeping).
// Set to 0 to only print timing on stability commit.
#ifndef DEBUG_TIMING_STREAM
  #define DEBUG_TIMING_STREAM 0   // CHANGED: default OFF for normal operation
#endif

// Set to 1 to also print raw samples a/b/c for each step (noisy; good for calibration).
#ifndef DEBUG_TIMING_VERBOSE
  #define DEBUG_TIMING_VERBOSE 0
#endif

namespace {
  // Runtime-selected pin (bound from Radio_Main.ino)
  uint8_t RC_PIN = Config::PIN_TUNING_INPUT;

  // ---- Timing knobs (adjust if needed) ----
  const uint16_t DISCHARGE_MS  = 10;
  const uint32_t TIMEOUT_US    = 2000000;   // 2 s safety

  // ---------------- Bands (µs) ----------------
  const uint32_t F03_LOWER_US = 0;
  const uint32_t F03_UPPER_US = 30;

  const uint32_t F02_LOWER_US = 41;
  const uint32_t F02_UPPER_US = 51;

  const uint32_t F01_LOWER_US = 65;
  const uint32_t F01_UPPER_US = 89;

  const uint32_t F00_LOWER_US = 123;        // >=123 → folder 00 (no upper bound)

  // ---------------- Stability counts ----------------
  const uint8_t  STABLE_COUNT_FOLDER = 4;    // required hits for 00/01/02/03
  const uint8_t  STABLE_COUNT_GAP    = 4;    // required hits for 99

  // ---------------- State ----------------
  String  currentFolder = "99";              // committed folder/gap ("00".."03","99","FAULT")
  String  pendingClass  = "99";              // candidate bucket from last sample
  uint8_t pendingHits   = 0;                 // consecutive matches for candidate

  // ---------------- Measurement (median of 3) ----------------
  inline uint32_t measureRC_us_once() {
    // Discharge the node
    pinMode(RC_PIN, OUTPUT);
    digitalWrite(RC_PIN, LOW);
    delay(DISCHARGE_MS);

    // Measure charge time to digital HIGH (NO pullups)
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
      Serial.print(" b=");        Serial.print(b);
      Serial.print(" c=");        Serial.println(c);
    #endif

    // median(a,b,c)
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
  }

  // ---------------- Bucket classification (µs) ----------------
  inline String classifyBuckets(uint32_t t) {
    // dead gaps (between bands)
    if (t > F03_UPPER_US && t < F02_LOWER_US) return "99";
    if (t > F02_UPPER_US && t < F01_LOWER_US) return "99";
    if (t > F01_UPPER_US && t < F00_LOWER_US) return "99";

    // bands
    if (t >= F03_LOWER_US && t <= F03_UPPER_US) return "03";
    if (t >= F02_LOWER_US && t <= F02_UPPER_US) return "02";
    if (t >= F01_LOWER_US && t <= F01_UPPER_US) return "01";
    if (t >= F00_LOWER_US) return "00";                      // no upper bound

    return "99"; // tuning/outside
  }

  // Map class String to numeric folder 0..3 (99 for gap; 255 for FAULT)
  inline uint8_t classToNumeric(const String& cls) {
    if (cls == "00") return 0;
    if (cls == "01") return 1;
    if (cls == "02") return 2;
    if (cls == "03") return 3;
    if (cls == "99") return 99;
    if (cls == "FAULT") return 255;
    return 99; // default to gap
  }

  inline void printFolderColumn(const String& clsOrCommitted) {
    if (clsOrCommitted == "FAULT") {
      Serial.print("FAULT");
      return;
    }
    uint8_t num = classToNumeric(clsOrCommitted);
    if (num <= 3) {
      if (num < 10) Serial.print('0');
      Serial.print(num);
    } else {
      Serial.print(num); // 99 for gap
    }
  }

  // ---------------- One step: measure + stability + prints ----------------
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
      pendingHits  = 0;
      return;
    }

    String cls = classifyBuckets(t_us);

    // stability commit
    if (cls == pendingClass) {
      pendingHits++;
    } else {
      pendingClass = cls;
      pendingHits  = 1;
    }

    const bool    isGap = (cls == "99");
    const uint8_t need  = isGap ? STABLE_COUNT_GAP : STABLE_COUNT_FOLDER;

    if (cls != currentFolder && pendingHits >= need) {
      currentFolder = cls;

      Serial.print("folder="); Serial.println(currentFolder);

      Serial.print("t_us="); Serial.print(t_us);
      Serial.print(" cls="); Serial.print(cls);
      Serial.print(" hits="); Serial.println(pendingHits);
    }
  }

  inline uint8_t toNumericFolder(const String& s) {
    if (s == "00") return 0;
    if (s == "01") return 1;
    if (s == "02") return 2;
    if (s == "03") return 3;
    if (s == "99") return 99;
    if (s == "FAULT") return 255;
    return 99; // default to gap
  }
} // anonymous namespace

uint8_t RadioTuning::getFolder(uint8_t digitalPin) {
  RC_PIN = digitalPin;
  stepFolderSelect();
  return toNumericFolder(currentFolder);
}
