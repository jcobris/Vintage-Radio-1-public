
#pragma once
#include <Arduino.h>

// MP3 control for DY-SV5W (UART control)
//
// Folder scheme (logical, owned by main sketch):
//   0..3  = your 4 SD folders (logical)
//   99    = mute / gap (special state)
//
// Mapping:
//   logical 0..3 -> module folder 1..4 (logical + 1)
//
// Usage:
//   - Call MP3::init() once in setup()
//   - In loop() when MP3 mode selected:
//       MP3::setDesiredFolder(folder);
//       MP3::tick();

namespace MP3 {
  void init();                             // call from main setup()
  void tick();                             // call from main loop() (only in MP3 mode)
  void setDesiredFolder(uint8_t folder);   // 0..3 or 99=mute
  uint8_t getDesiredFolder();              // optional debug visibility
}
