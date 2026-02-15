# Vintage Radio Retrofit — Requirements (Draft)

**Owner:** Jeff Cornwell  
**Status:** Draft (intended to be revised)  
**Target MCU:** Arduino Nano (ATmega328P) first; ESP32 only if required  
**Primary Goal:** Retrofit a vintage radio with modern Bluetooth + MP3 playback and synchronized lighting/display effects, while preserving original user controls and aesthetics.

---

## 1. Project Overview

This project integrates:
- A Bluetooth audio receiver module (BT210)
- A UART-controlled MP3 playback module (DY-SV5W)
- A WS2812B 8x32 LED matrix for animated themes
- An analog (PWM) dial/tuner light strip
- A tuning capacitor timing input to select MP3 folders (1–4)
- A physical audio source selector (2-pole switch) that routes audio to the preamp
- A 3-position display mode switch to control display behavior (normal / reserved / matrix-off)

**Key concept:**  
Audio selection is physical (2-pole switch). Firmware detects which source is selected and drives the LED matrix + dial lights accordingly.

---

## 2. Hardware & Interfaces

### 2.1 Microcontroller
- **Primary:** Arduino Nano (ATmega328P)
- **Fallback:** ESP32 if animations/logic exceed Nano resources

### 2.2 Modules

#### MP3 Module
- **Model:** DY-SV5W  
- **UART control:** SoftwareSerial  
- **Pins:** Nano pins **12, 11** (as wired/verified; TX/RX direction documented in PINOUT)  
- **Baud:** **9600**  
- **Behavior note:** “Play random within folder” always begins at track 1.  
  - Each folder will include a **short blank MP3 as Track 1** so the first *audible* track is effectively random.

#### Bluetooth Module
- **Model:** BT210  
- **UART control:** SoftwareSerial  
- **Pins:** Nano pins **9, 10** (as wired/verified; TX/RX direction documented in PINOUT)  
- **Baud:** **TBD / verify**  
  - Notes from build info contain both **57600** and **9600** references. The final spec will record the tested working baud once confirmed.

> **Datasheets** (to add later): place PDFs in `docs/datasheets/`  
> - `docs/datasheets/DY-SV5W.pdf`  
> - `docs/datasheets/BT210.pdf`

---

## 3. LEDs / Outputs

### 3.1 LED Matrix
- **Type:** WS2812B  
- **Size:** 8 x 32  
- **Data pin:** **D7**  
- **Animation constraint:** **Single color per 8-LED column** (columns treated as blocks; one RGB color per column group at a time)

### 3.2 Dial / Tuner Light (Analog PWM)
- **Type:** Analog LED strip driven via PWM (through appropriate driver circuit)  
- **PWM pin:** **D6**  
- **Normal behavior:** On (details vary by display switch and folder theme; see Modes)

---

## 4. Inputs / Controls

### 4.1 Source Selection Sense (Physical 2-pole switch awareness)
- **Sense pin:** **D2**
- **Logic:**
  - **LOW (grounded)** → MP3 is the selected audio source
  - **HIGH (internal pull-up)** → Bluetooth is the selected audio source
- Firmware uses this only for display/lighting behavior.

### 4.2 Display Mode Switch (3-position)
A 3-position switch selects display behavior; pins **D3/D4/D5** indicate which position is active.

- **D3 active:** Normal operation (display themes as specified)
- **D4 active:** Reserved (currently does nothing; future theme expansion)
- **D5 active:** Display override
  - LED matrix OFF
  - Dial/tuner light ON solid (fixed brightness)

> Implementation detail (not required by this spec, but implied): likely `INPUT_PULLUP` and the active pin reads LOW.

### 4.3 Folder Selection via Tuning Capacitor Timing
- **Input pin:** **D1** (timing measurement input as wired)
- A function measures how long the tuning capacitor takes to charge.
- The measured time maps to **MP3 folder number 1–4**.

**Folder mapping**
- Output: `folder = 1..4`
- Thresholds: **TBD** (calibration required)

---

## 5. Modes & System Behavior

### 5.1 Display Behavior Priority (override hierarchy)
1. If **Display Switch = D5 active** → Matrix OFF; tuner light solid ON
2. Else if **Display Switch = D3 active** → Normal display logic (based on D2 + folder)
3. Else if **Display Switch = D4 active** → Reserved (no special behavior yet; placeholder)

### 5.2 Normal Display Logic (when D3 active)

#### Bluetooth Selected (D2 HIGH)
- Display theme: **Party / Rainbow Marble**
- **Party pattern runs continuously** while Bluetooth is the selected source.

#### MP3 Selected (D2 LOW)
- Folder is determined by tuning capacitor timing (1–4)
- Display themes by folder:
  - **Folder 1:** Party / Rainbow Marble
  - **Folder 2:** Fire
  - **Folder 3:** Christmas (half red/green pulsing)
  - **Folder 4:** Eerie green/blue slow pulsing
    - Dial/tuner light: synced breathing/pulse (yellow dial illumination pulses in sync)

---

## 6. MP3 Playback Control Requirements

When the desired folder changes (based on tuning capacitor timing), firmware must perform the following sequence:

1) **Mute volume**  
2) Navigate folders by sending **next folder** or **previous folder** commands until desired folder is reached  
3) Send **play random in folder** command  
   - Note: module starts at Track 1; folders include blank Track 1 to make audible start random  
4) Send **Play** command  
5) Set **Volume to max** (or configured default)

**Command timing**
- Inter-command delays: **TBD** (per datasheet/testing)
- Volume scale values (mute/max): **TBD** (per module spec)

---

## 7. Performance & Quality Requirements (Non-Functional)

- Must be stable for long runtimes (target: 2+ hours continuous)
- Avoid blocking delays where possible (animations should not freeze input detection)
- Prefer minimal RAM usage (Nano constraints)
- Debug output desired where feasible (Serial monitoring)

---

## 8. Diagnostics / Debug Requirements
- Preferred debug baud: **115200** over USB serial (if available)
- Debug messages should include:
  - Selected source state (BT/MP3 based on D2)
  - Measured capacitor timing value
  - Mapped folder number (1–4)
  - MP3 command sequence steps

---

## 9. Acceptance Criteria (Initial)

**AC1 — Source detect**
- Switching the physical selector results in correct BT/MP3 detection (via D2) and correct display theme update quickly (target: < 250ms perceived).

**AC2 — Folder selection**
- Multiple tuning knob positions reliably map to folders 1–4 (target: ≥ 9/10 correct across quick tests once calibrated).

**AC3 — MP3 command sequence**
- Folder change triggers the full sequence:
  - mute → navigate → random-in-folder → play → volume max
- Audible playback starts cleanly (blank Track 1 prevents repeated audible first track).

**AC4 — Display mode switch**
- D3: normal themes work as specified
- D5: matrix turns off and tuner light becomes solid on
- D4: no visible behavior change (reserved) but does not break anything

**AC5 — Pattern correctness**
- BT mode: party pattern runs continuously while BT selected
- MP3 folders show correct patterns (F1 party, F2 fire, F3 Christmas, F4 eerie + synced dial pulse)
- No obvious flicker or lockups during normal operation

---

## 10. Open Items / TBD (Expected to be refined)
- Exact BT210 UART baud (confirm tested working value)
- Capacitor timing thresholds for folder mapping (calibration values)
- Confirm direction of SoftwareSerial wiring (Nano TX/RX to module RX/TX)
- Dial light PWM brightness level for “solid on” override (D5)
- Potential pin conflict considerations when using D1 for timing input (see PINOUT notes)
- Final decision on animation library (FastLED vs Adafruit_NeoPixel)