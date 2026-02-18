
#pragma once
#include <Arduino.h>

// MP3 control for DY-SV5W (UART control)
//
// Folder scheme (standardized):
//   1..4  = SD folders 1..4
//   99    = mute / gap
//
// Usage:
//   - Call MP3::init() once in setup()
//   - In loop() when MP3 mode selected:
//       MP3::setDesiredFolder(folder);
//       MP3::tick();

namespace MP3 {
  void init();
  void tick();
  void setDesiredFolder(uint8_t folder); // 1..4 or 99=mute
  uint8_t getDesiredFolder();
}
