
// Radio_Tuning.h
#pragma once
#include <Arduino.h>
#include "Config.h" // pins + global debug macros

namespace RadioTuning {
  // Measures RC timing on the given digital pin and returns:
  // 1..4 = stable folder selection (standardized mapping)
  // 99  = gap / between bands
  // 255 = FAULT (timeout / measurement failure)
  uint8_t getFolder(uint8_t digitalPin);
}
