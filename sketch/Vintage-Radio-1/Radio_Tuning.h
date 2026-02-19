
// Radio_Tuning.h
#pragma once
#include <Arduino.h>
#include "Config.h" // pins + global debug macros

namespace RadioTuning {
  // Measures RC timing on the given digital pin and returns:
  // 1..4 = stable folder selection (standardized mapping)
  // 99   = gap / between bands
  // 255  = FAULT (timeout / measurement failure)
  uint8_t getFolder(uint8_t digitalPin);

  // Returns the most recent instantaneous classification from the last measurement
  // (1..4, 99 gap, or 255 fault). This does NOT require the stable/commit logic.
  uint8_t getInstantClass();
}
