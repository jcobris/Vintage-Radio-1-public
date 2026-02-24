
// MP3.h
#pragma once
#include <Arduino.h>

/*
  ============================================================
  MP3 Control (DY-SV5W)
  ============================================================

  This module controls a DY-SV5W MP3 player via UART (SoftwareSerial).

  Folder scheme (Option A):
  - 1..4 : SD folders 1..4 (real content folders)
  - 99   : "gap/mute" - when requested, the module volume is set to 0

  Usage:
  - Call MP3::init() once in setup()
  - In loop() while MP3 source is selected:
      MP3::setDesiredFolder(folder);   // 1..4 or 99
      MP3::tick();                     // non-blocking state machine

  Key behaviour:
  - When desired folder changes, actions are performed ONCE per change
    (folderSelected gating).
  - When desired=99, send volume=0.
  - When leaving 99 and starting playback again, volume is restored
    before play (see MP3.cpp playRandomTrack()).
*/

namespace MP3 {
  void init();
  void tick();
  void setDesiredFolder(uint8_t folder); // 1..4 or 99=mute
  uint8_t getDesiredFolder();
}
