# Vintage Radio Retrofit — Architecture (Draft)

**Owner:** Jeff Cornwell  
**Status:** Draft (edit as implementation evolves)  
**Target MCU:** Arduino Nano (ATmega328P) first; ESP32 fallback if required  
**Purpose:** Describe software structure, state model, and runtime behavior so the final Arduino sketch remains maintainable and testable.

---

## 1. High-Level Design Goals

1) Keep `loop()` responsive (avoid long blocking delays).  
2) Preserve stability on Nano (RAM/CPU conscious; avoid dynamic Strings).  
3) Modularize subsystems so test code can be merged safely:
   - Input sensing (source + display switch)
   - Folder selection (capacitor timing)
   - MP3 control (DY-SV5W)
   - LED matrix patterns (WS2812 8x32)
   - Dial/tuner PWM lighting
4) Make “what should the display do right now?” deterministic via a clear priority model.

---

## 2. Runtime State Model

### 2.1 Primary Inputs -> Derived State

**Inputs**
- D2: Source sense (BT vs MP3)
- D3/D4/D5: Display mode switch position
- D8: Capacitor timing measurement -> folder (1–4)

**Derived state**
- `sourceMode`:
  - `SOURCE_BT` when D2 HIGH
  - `SOURCE_MP3` when D2 LOW
- `displayMode`:
  - `DISPLAY_NORMAL` when D3 active
  - `DISPLAY_RESERVED` when D4 active
  - `DISPLAY_OFF` when D5 active
- `folder` (1..4) from capacitor timing thresholds (calibration constants)

### 2.2 Display Priority / Overrides

**Priority rule**
1) If `DISPLAY_OFF` (D5 active):
   - Matrix is OFF
   - Tuner light forced solid ON (fixed PWM)
   - No need to animate matrix patterns
2) Else if `DISPLAY_NORMAL` (D3 active):
   - Render patterns based on `sourceMode` and `folder`
3) Else if `DISPLAY_RESERVED` (D4 active):
   - Placeholder: currently behave like normal OR show nothing (TBD)
   - Must not break other subsystems

### 2.3 Effective Theme Selection

When `DISPLAY_NORMAL`:

- If `SOURCE_BT`:
  - Theme = `THEME_PARTY`
  - Party animation runs continuously while BT is selected
- If `SOURCE_MP3`:
  - Theme depends on `folder`:
    - folder 1 -> `THEME_PARTY`
    - folder 2 -> `THEME_FIRE`
    - folder 3 -> `THEME_XMAS`
    - folder 4 -> `THEME_EERIE` (sync dial PWM pulse to theme)

---

## 3. Subsystems (Modules) and Responsibilities

### 3.1 InputManager
**Responsibilities**
- Read D2, D3/D4/D5
- Apply debounce / stability window where needed
- Expose `sourceMode`, `displayMode`
- Detect change events (e.g., source changed)

**Notes**
- Use `INPUT_PULLUP` on D2/D3/D4/D5 and treat LOW as active where applicable.
- Switch state should be stable for N ms before accepting (TBD constant).

### 3.2 FolderSelector (Capacitor Timing)
**Responsibilities**
- Measure capacitor charge time via D8 circuit
- Convert measurement into folder 1–4 using threshold constants
- Expose `folder`
- Detect folder change events

# Vintage Radio Retrofit — Architecture

**Owner:** Jeff Cornwell  
**Status:** Current / Accurate  
**Target MCU:** Arduino Nano (ATmega328P)

---

## 1. Architectural Intent

This project is intentionally simple and deterministic.

Primary goals:

1. **Stability on Arduino Nano**
   - No dynamic allocation
   - No `String`
   - Predictable timing
   - Long‑running stability

2. **Preserve physical control behaviour**
   - Audio switching is physical
   - Firmware reacts to hardware state
   - No “soft” overrides of user intent

3. **Non‑blocking main loop**
   - No long delays in `loop()`
   - All time‑sensitive work spread over iterations

4. **Minimal abstraction**
   - One main `.ino`
   - Hardware‑focused modules
   - No background schedulers or RTOS concepts

This document reflects **what the system does today**, not future ideas.

---

## 2. High‑Level Structure

The firmware consists of:

- One **orchestrating sketch** (`.ino`)
- Several **single‑responsibility modules**

Inputs ──▶ Main .ino ──▶ Outputs
│
├─ Radio_Tuning
├─ MP3
├─ Bluetooth
├─ LedMatrix
├─ LedStrip
└─ DisplayLED

There is no asynchronous processing.
Everything is driven directly from `loop()`.

---

## 3. Inputs

All inputs are read synchronously in the main loop.

| Pin | Purpose |
|---|---|
| D2 | Source detect (MP3 / Bluetooth) |
| D3 | Display mode: Normal |
| D4 | Display mode: Reserved |
| D5 | Display mode: Matrix OFF |
| D8 | Tuning capacitor timing input |
| D13 | MP3 next‑track button |

---

## 4. Tracked State (Main `.ino`)

The main sketch maintains:

- `sourceMode`
  - `SOURCE_MP3`
  - `SOURCE_BT`
- `displayMode`
  - `DISPLAY_NORMAL`
  - `DISPLAY_ALT` (reserved)
  - `DISPLAY_OFF`
- `folder`
  - `1..4` valid
  - `99` gap
- Button debounce state
- Timing state (poll intervals)

All state transitions are **edge‑detected** by comparing current vs previous values.

---

## 5. Display Priority Model

Display behaviour follows a strict priority order.

### Priority 1 — Display OFF (D5)
- LED matrix OFF
- Dial/tuner LED forced solid ON
- No matrix animation updates occur

### Priority 2 — Normal Display (D3)
- Matrix + dial LEDs active
- Behaviour depends on source and folder

### Priority 3 — Reserved (D4)
- Currently behaves the same as normal
- Placeholder for future expansion
- Must not alter existing behaviour

---

## 6. Source‑Dependent Behaviour

### 6.1 Bluetooth Selected (D2 HIGH)

- MP3 subsystem idle
- Bluetooth module awake if required
- LED matrix runs **Party / Rainbow** theme continuously
- Folder selection ignored
- Dial LED at normal solid brightness

---

### 6.2 MP3 Selected (D2 LOW)

- Tuning capacitor selects folder
- MP3 module follows folder selection
- Display theme depends on folder:

| Folder | Meaning | Theme |
|---|---|---|
| 1 | Valid | Party |
| 2 | Valid | Fire |
| 3 | Valid | Christmas |
| 4 | Valid | Eerie |
| 99 | Gap | Between‑stations flicker |
| 255 | Fault | Treated as gap |

---

## 7. Module Responsibilities

### 7.1 `Radio_Tuning`

- Measures RC timing on D8
- Median‑of‑three sampling
- Maps timing to folder / gap / fault
- Applies stability (anti‑flicker) logic
- Exposes:
  - Committed folder
  - Instant classification (visual use)

The main sketch consumes **only the committed folder**.

---

### 7.2 `MP3`

- Controls DY‑SV5W via UART
- Executes folder‑change sequence:
  1. Mute
  2. Navigate folders
  3. Random‑in‑folder
  4. Play
  5. Restore volume
- Handles next‑track command

The MP3 module acts only on **changes**.

---

### 7.3 `Bluetooth`

- Performs optional AT‑command setup
- UART passthrough only when explicitly enabled
- Normally asleep during runtime

Bluetooth serial reception is not required for normal operation.

---

### 7.4 `LedMatrix`

- Drives WS2812B 8×32 matrix
- Renders non‑blocking animations
- Enforces “one colour per 8‑LED column” rule
- Calls `FastLED.show()`

Matrix updates are **skipped during RC timing polls**
to avoid interrupt interference.

---

### 7.5 `LedStrip`

- Drives additional WS2812B strip
- Generates pattern data only
- Never calls `FastLED.show()`

Actual output occurs when the matrix updates.

---

### 7.6 `DisplayLED`

- Drives analog dial/tuner LEDs via PWM
- Provides:
  - Solid brightness
  - Random flicker
  - Sine‑based pulse

Used heavily for:
- Between‑stations feedback
- Eerie theme synchronisation

---

## 8. Main Loop Execution Order

Each `loop()` iteration follows this sequence:

1. Read display mode switch
2. Read source detect
3. Handle next‑track button
4. Poll tuning capacitor (timed)
5. Update MP3 subsystem
6. Update dial LED
7. Update LED strip
8. Update LED matrix (if safe)

RC timing polls deliberately skip LED updates for that iteration.

---

## 9. Serial & Debug Strategy

- Serial initialised once in `.ino`
- Debug is compile‑time controlled
- Subsystem‑specific toggles
- No runtime overhead when disabled

---

## 10. Constraints

- Arduino Nano resources are sufficient
- No dynamic memory
- One SoftwareSerial active at a time
- WS2812 timing respected
- Physical controls are authoritative