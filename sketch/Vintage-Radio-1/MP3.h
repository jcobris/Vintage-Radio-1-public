
#pragma once
#include <Arduino.h>

// MP3 control for DY-SV5W
//
// Desired folder scheme (logical):
//   0..3  = folder selections (your 4 folders)
//   99    = mute / gap (special "silent" state)
// Notes:
//   - Mapping of logical folder (0..3) to actual SD folder numbering
//     is handled by the existing next/prev folder stepping logic.
//   - Main sketch should call setDesiredFolder() before tick().

namespace MP3 {
  void init();                         // call from main setup()
  void tick();                         // call from main loop() when MP3 selected
  void setDesiredFolder(uint8_t folder); // 0..3, or 99=mute/gap
  uint8_t getDesiredFolder();          // for debug/visibility if needed
}
