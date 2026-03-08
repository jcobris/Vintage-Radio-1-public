
# Vintage Radio Retrofit (Arduino Nano)

**Owner:** Jeff Cornwell  
**Status:** Active build  
**Target MCU:** Arduino Nano (ATmega328P)

---

## Overview

This project retrofits a vintage radio with modern audio playback and synchronized lighting effects while preserving the original look and physical controls.

### Features
- Bluetooth audio playback
- MP3 playback from SD card (folder‑based)
- Physical audio source selection
- LED matrix animations
- Dial/tuner LED illumination
- Tuning knob used as MP3 folder selector
- Next‑track push button
- Display mode override switch

---

## Audio Sources

### Bluetooth
- Module: BT210
- UART: SoftwareSerial
- Used only for audio playback
- Configuration persists across power cycles

### MP3
- Module: DY‑SV5W
- Plays audio from SD card folders 1–4
- Folder selection driven by tuning capacitor timing
- “Random in folder” playback strategy

---

## Controls

### Source Selection
- Physical 2‑pole switch routes audio
- Firmware detects selected source via pin D2

### Display Mode Switch
- 3‑position switch:
  - Normal operation
  - Reserved (future)
  - Matrix OFF, dial light solid ON

### Tuning Knob
- Capacitor timing measurement
- Selects MP3 folder (1–4)
- Gap positions treated as “between stations”

### Next‑Track Button
- Active when MP3 is selected
- Sends “next track” command to MP3 module

---

## Displays

### LED Matrix
- WS2812B 8×32 matrix
- One color per 8‑LED column
- Theme depends on source + folder

### Dial / Tuner LEDs
- PWM‑driven LED strip
- Solid, flicker, or pulsing depending on mode/theme

---

## Themes

| Mode | Behaviour |
|----|----|
| Bluetooth | Party / rainbow animation |
| MP3 Folder 1 | Party |
| MP3 Folder 2 | Fire |
| MP3 Folder 3 | Christmas |
| MP3 Folder 4 | Eerie (slow pulse + synced dial light) |

---

## Build Notes

- Designed for long‑term stability
- Avoids dynamic memory
- Debug output is compile‑time configurable
- Pin conflicts prevented with static assertions

---

## Status

The project is currently stable and fully functional.  
Future work focuses on visual refinement and noise reduction.
