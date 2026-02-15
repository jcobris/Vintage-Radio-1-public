# Vintage Radio Retrofit (Arduino Nano) — README

**Owner:** Jeff Cornwell  
**Status:** Active build  
**Target board:** Arduino Nano (ATmega328P)  
**Fallback board:** ESP32 (only if Nano resources prove insufficient)

## Overview

This project retrofits a vintage radio with modern playback sources and LED effects:

- **Bluetooth audio module:** BT210 (UART used only for optional setup; settings persist across reboot)
- **MP3 module:** DY-SV5W (UART control; plays from SD card folders 1–4)
- **Audio source selection:** Physical 2‑pole switch routes audio to the preamp (code does not switch audio)
- **Source detection:** Pin D2 senses which source is selected so the correct display theme runs
- **Folder selection:** Tuning capacitor charge-time measurement maps to folder 1–4
- **Display outputs:**
  - WS2812B **8x32 matrix** themes
  - Analog **dial/tuner LED strip** via PWM

## Key Behaviors

### Source detection (D2)
- **D2 LOW (grounded)** → MP3 selected → show MP3 folder themes
- **D2 HIGH (pull-up)** → Bluetooth selected → show party theme

### Display mode switch (D3/D4/D5)
A 3-position switch controls the display system:

- **D3 active:** Normal operation (themes as specified)
- **D4 active:** Reserved (no behavior yet; future themes)
- **D5 active:** Matrix OFF, tuner light solid ON

### Themes (Normal operation when D3 active)
- **Bluetooth selected:** Party / rainbow marble runs continuously (while BT is selected)
- **MP3 selected:** Theme depends on folder:
  - Folder 1 → Party / rainbow marble
  - Folder 2 → Fire
  - Folder 3 → Christmas (red/green half pulsing)
  - Folder 4 → Eerie green/blue slow pulse + dial light synced pulse

**Matrix constraint:** one color per 8‑LED column (column/block color rule).

### MP3 folder change command sequence
When the desired folder changes (from tuning capacitor timing):

1) mute volume  
2) send next/prev folder commands until desired folder reached  
3) send “random in folder” (starts at track 1)  
4) send play command  
5) set volume to max  

> Note: each folder has a short blank MP3 as track 1 so the first audible track is effectively random.

---

## Repo Layout (suggested)

