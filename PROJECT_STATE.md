
# Project State — Vintage Radio Retrofit

**Owner:** Jeff Cornwell  
**Status:** Stable / Active  
**Target MCU:** Arduino Nano (ATmega328P)

---

## Current State (Summary)

The project is fully functional and stable in its current form.

All core systems are working:
- Bluetooth audio playback
- MP3 playback with folder selection via tuning capacitor timing
- LED matrix animations
- Dial/tuner LED illumination
- Display mode override switch
- Next‑track button for MP3

This phase of work focused on:
- Finalising debug behaviour (compile‑time, predictable, non‑spammy)
- Locking in pin usage with compile‑time guardrails
- Preparing documentation for future visual/animation refinement

---

## Hardware Configuration (Verified)

### Audio Sources
- **Bluetooth module:** BT210  
  - SoftwareSerial @ 57600 baud  
  - Pins: D9 (TX), D10 (RX)
- **MP3 module:** DY‑SV5W  
  - SoftwareSerial @ 9600 baud  
  - Pins: D11 (TX), D12 (RX)

Audio switching is **physical** via a 2‑pole selector.  
Firmware only *detects* the selected source.

### Inputs
- **D2:** Source detect  
  - LOW = MP3  
  - HIGH = Bluetooth (INPUT_PULLUP)
- **D3 / D4 / D5:** Display mode switch  
  - D3 = Normal  
  - D4 = Reserved (future use)  
  - D5 = Matrix OFF, dial light solid ON
- **D8:** Tuning capacitor timing input (folder selection)
- **D13:** MP3 next‑track button (active‑low, debounced)

### Outputs
- **D7:** WS2812B 8×32 LED matrix
- **D6:** Dial/tuner LED strip (PWM)
- **A0:** Additional WS2812B LED strip

---

## Folder / Theme Behaviour

Folder selection maps to MP3 folders and display themes:

| Folder | Meaning | Theme |
|------|--------|------|
| 1 | Valid | Party |
| 2 | Valid | Fire |
| 3 | Valid | Christmas |
| 4 | Valid | Eerie |
| 99 | Gap | Between‑stations flicker |
| 255 | Fault | Treated as gap |

---

## Known Next Work

Planned follow‑up tasks:
- Reduce audible PWM noise from dial/tuner LEDs
- Refine LED matrix animations (party + eerie)
- Refine LED strip patterns
- Document final calibration values once locked

---

## Stability Notes

- System runs for extended periods without lockups
- No dynamic memory usage (`String` avoided)
- Debug output is compile‑time controlled and predictable
