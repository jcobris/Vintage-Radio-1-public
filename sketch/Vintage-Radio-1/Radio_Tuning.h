
// Radio_Tuning.h
#pragma once
#include <Arduino.h>
#include "Config.h"

/*
  ============================================================
  Radio Tuning (RC timing -> folder mapping)
  ============================================================

  The tuning knob / mechanism produces distinct RC charge-time regions for
  4 stable positions plus dead spaces between them.

  Measurement method:
  - Discharge the RC node by forcing pin LOW as OUTPUT for a short time.
  - Switch the pin to INPUT and measure how long it takes to read HIGH.
  - Use median-of-three sampling for noise rejection.

  Output values:
  - 1..4 : Stable folder (Option A mapping)
  - 99   : Gap / dead space between bands
  - 255  : Fault (timeout / measurement failure)

  Two concepts are tracked:
  - "Committed" folder: stable result after repeated matching classifications.
  - "Instant" class: classification of the most recent measurement,
    which can briefly hit 99 during motion or jitter.
*/

namespace RadioTuning {

  // Measure the tuning RC timing on the given digital pin and update internal
  // state. Returns the committed result (stable folder / gap / fault).
  uint8_t getFolder(uint8_t digitalPin);

  // Returns the most recent instantaneous classification from the last
  // measurement. This does NOT require stability/commit and may flicker.
  uint8_t getInstantClass();
}
