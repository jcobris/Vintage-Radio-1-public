# Vintage Radio Retrofit (Arduino Nano)

Personal project: retrofit a vintage radio with modern playback sources + LED effects

## Current sketch (open this in Arduino IDE)
- `sketch/Vintage-Radio-1/Vintage-Radio-1.ino`
  - This folder also contains supporting modules (`*.cpp` / `*.h`) used by the sketch.

## What it does (high level)
- Bluetooth audio module (BT210)
- MP3 module (DY-SV5W) controlled by UART; SD folders 1–4
- Physical 2‑pole switch selects audio (code only detects selection)
- WS2812B 8x32 matrix themes + dial/tuner light
- Tuning knob timing maps to folder 1–4

## Docs
- Start at `ASSISTANT_START_HERE.md`
- Then `PROJECT_STATE.md`
- Design notes in `docs/`