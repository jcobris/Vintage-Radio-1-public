We’re continuing my Vintage Radio Retrofit project. I’m using Arduino IDE targeting Arduino Nano (ESP32 fallback only if necessary). Please work one step at a time.

Project docs are in my repo:
- docs/REQUIREMENTS.md
- docs/ARCHITECTURE.md
- docs/PINOUT.md
- docs/README.md
- PROJECT_STATE.md

Current hardware/pins:
- MP3 DY-SV5W @9600 on SoftwareSerial pins 12,11
- BT210 on SoftwareSerial pins 9,10 (setup-only; 2 messages on pause/play)
- Matrix WS2812B 8x32 on D7
- Tuner PWM lights on D6
- Source sense on D2 (LOW=MP3, HIGH=BT)
- Display switch: D3 normal, D4 reserved, D5 matrix off + tuner light solid on
- Tuning capacitor timing input on D1 maps to folder 1–4
- Patterns: BT/F1 party continuous; F2 fire; F3 Christmas half pulsing; F4 eerie pulsing synced with tuner
- MP3 folder change sequence: mute -> step folder -> random-in-folder -> play -> volume max
- Matrix constraint: single color per 8-LED column

Next objective: help me merge my working test sketches into a modular final sketch (config.h + modules), starting with a clean skeleton that compiles.