# Project State — Vintage Radio Retrofit

## Current Goal
Combine all proven test sketches into one final Arduino Nano project.

## Hardware Summary
- MCU: Arduino Nano (ATmega328P), ESP32 fallback if needed
- MP3 module: DY-SV5W @ 9600 baud (SoftwareSerial pins 12,11)
- BT module: BT210 (SoftwareSerial pins 9,10), setup-only; emits 2 messages on pause/play; no continuous chatter
- Matrix: WS2812B 8x32 on D7
- Dial/tuner light: analog PWM on D6
- Source sense: D2 (LOW=MP3 selected, HIGH=BT selected)
- Display switch: D3 normal themes, D4 reserved, D5 matrix off + tuner light solid on
- Tuning capacitor timing input: D1 (maps charge time to folder 1–4)

## Display/Themes
- BT selected: PARTY pattern runs continuously
- MP3 selected:
  - Folder 1: PARTY
  - Folder 2: FIRE
  - Folder 3: CHRISTMAS red/green half pulsing
  - Folder 4: EERIE green/blue slow pulsing + dial PWM synced
- Matrix constraint: single color per 8-LED column

## MP3 Folder Change Sequence
1) Mute
2) next/prev folder until desired
3) random-in-folder (starts at track 1; blank track1 in each folder)
4) play
5) volume max

## Known Notes / Risks
- Nano D1 is hardware Serial TX (USB). Using D1 for timing input may affect uploading/debugging.

## What’s Working Today
- (Fill in) Links to test sketches that are verified working:
  - tests/...
  - tests/...

## Next Steps
- Create final sketch skeleton + config.h
- Merge subsystems one at a time (inputs → MP3 → matrix → dial PWM)
- Add thresholds later from existing working config