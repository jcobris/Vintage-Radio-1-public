# Vintage Radio Retrofit — Pinout (Draft)

**Owner:** Jeff Cornwell  
**Status:** Draft (edit as wiring evolves)  
**Target:** Arduino Nano (ATmega328P)

> This document records the physical wiring pins and electrical notes.
> Direction (TX/RX) is documented explicitly for serial links where needed.

---

## 1. Digital Inputs

### D2 — Source Select Sense (2-pole audio selector awareness)
- **Purpose:** Detect which audio source is physically routed to the preamp
- **Logic:**
  - LOW (ground) = MP3 selected
  - HIGH (pull-up) = Bluetooth selected
- **Recommended config:** INPUT_PULLUP

### D3 / D4 / D5 — Display Mode 3-position Switch
Only one position should be active at a time.

- **D3:** Normal displays enabled
- **D4:** Reserved (future themes)
- **D5:** Matrix OFF, tuner light solid ON
- **Recommended config:** INPUT_PULLUP (active = LOW)

---

## 2. Tuning / Folder Selection Input

### D1 — Tuning Capacitor Timing Input
- **Purpose:** Measure capacitor charge time to map to folder 1–4
- **Signal type:** timing-based digital measurement (details depend on circuit)

⚠️ **Important note (Nano hardware serial conflict):**
- Arduino Nano **D0/D1 are the hardware Serial (USB) RX/TX pins**.
- Using **D1** for capacitor timing can interfere with USB Serial debugging and uploading behavior.
- If you see upload/debug issues, options include:
  - Move tuning measurement to a different pin (often an analog pin like A0)
  - Reduce/disable Serial debug in final build
  - Ensure the timing circuit does not fight the USB serial interface electrically

(Keep as-is for now since your wiring is verified; this is just a documented risk.)

---

## 3. LEDs / Outputs

### D6 — Dial/Tuner Lights (PWM Output)
- **Purpose:** PWM dimming for analog dial light strip
- **Notes:**
  - Typically requires a MOSFET/transistor driver depending on strip current
  - “Solid ON” behavior when D5 display override is active

### D7 — WS2812B Matrix Data
- **Purpose:** Data line for WS2812B 8x32 LED matrix
- **Notes:**
  - Ensure common ground between Nano and LED power supply
  - Consider series resistor on data line (e.g., 220–470Ω) and proper power injection as required

---

## 4. UART / SoftwareSerial Links

> These are listed as “pins used” per verified wiring.
> Add the exact direction once confirmed in your notes (Nano TX -> module RX etc.)

### Bluetooth Module (BT210) — SoftwareSerial
- **Pins:** D9, D10
- **Baud:** TBD/Verify (seen 57600 and 9600 references in notes)
- **Direction:** TBD (document as wired)
- **Notes:** Party pattern runs continuously while BT is selected.

### MP3 Module (DY-SV5W) — SoftwareSerial
- **Pins:** D12, D11
- **Baud:** 9600
- **Direction:** TBD (document as wired)
- **Notes:** Folder navigation + random-in-folder sequence used

---

## 5. Power / Ground (Fill in as final wiring is confirmed)
- **5V rail source:** TBD (Nano USB / regulator / external)
- **LED matrix power:** TBD (recommended external 5V supply sized for WS2812 load)
- **Grounding:** Must share common GND between:
  - Nano
  - LED matrix power supply
  - PWM dial light driver
  - MP3 + Bluetooth modules

---

## 6. Spare Pins / Future Expansion
- Any unused pins: TBD
- D4 is reserved for future display themes (behavior expansion)