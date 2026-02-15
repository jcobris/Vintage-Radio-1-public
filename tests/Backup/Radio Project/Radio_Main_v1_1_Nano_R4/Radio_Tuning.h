
// Radio_Tuning.h
#pragma once
#include <Arduino.h>
#include "Config.h"  // pins + global debug macros

namespace RadioTuning {
  // Measures RC timing on the given digital pin and returns numeric folder (0..3).
  // Internally keeps the original String-based state and stability logic intact.
  uint8_t getFolder(uint8_t digitalPin);
}
