# Project State — Vintage Radio Retrofit

## Current Goal
Combine proven test sketches into one final Arduino Nano project (ESP32 fallback only if required).

## Target / Tools
- IDE: Arduino IDE
- Board: Arduino Nano (ATmega328P)

## Hardware Summary (Verified)
- MP3 module: DY-SV5W @ 9600 baud
  - SoftwareSerial pins: (12,11)
- Bluetooth module: BT210
  - SoftwareSerial pins: (9,10)
  - Setup-only; retains config on reboot
  - Emits 2 messages on pause/play; no continuous chatter
- Matrix: WS2812B 8x32 on D7
- Dial/tuner light strip: Analog PWM on D6
- Source sense: D2 (LOW=MP3 selected, HIGH=BT selected)
- Display switch (3-position): D3 normal themes, D4 reserved, D5 matrix off + tuner light solid on
- Tuning capacitor timing input: D1 (maps charge time to folder 1–4)

## Display/Themes (Normal when D3 active)
- BT selected: PARTY pattern runs continuously
- MP3 selected:
  - Folder 1: PARTY
  - Folder 2: FIRE
  - Folder 3: CHRISTMAS (red/green half pulsing)
  - Folder 4: EERIE (green/blue slow pulsing + dial PWM synced)
- Constraint: Matrix uses a single color per 8-LED column

## MP3 Folder Change Sequence
1) Mute volume  
2) Send next/prev folder until desired folder reached  
3) Send play-random-in-folder (starts at track 1; blank track 1 in each folder)  
4) Send play  
5) Set volume max  

## Docs (Source of Truth)
- docs/REQUIREMENTS.md
- docs/ARCHITECTURE.md
- docs/PINOUT.md
- docs/README.md

## Known Risks / Notes
- Nano D1 is hardware Serial TX (USB). Using D1 for timing input may affect uploads/debugging.

## What’s Working Today
- (Fill in) Link or names of working test sketches under /tests
- (Fill in) Any known-good parameter values (timing thresholds, volumes, delays)

## Next Steps (Planned)
1) Create final sketch skeleton + config.h (compiles clean)
2) Merge subsystems one-by-one:
   - Inputs (D2 + D3/D4/D5)
   - Folder selector (use your known-good config)
   - MP3 controller state machine (non-blocking)
   - Matrix patterns
   - Dial PWM + sync for folder 4
3) Add GitHub Issues for each subsystem and acceptance tests