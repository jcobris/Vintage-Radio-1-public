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

**Notes / Risks**


**Calibration**
- Store thresholds in `config.h`:
  - `T_F1_MAX`, `T_F2_MAX`, `T_F3_MAX`, else folder 4
- Consider smoothing:
  - sample multiple readings; take median/average; add hysteresis to avoid chatter.

### 3.3 Mp3Controller (DY-SV5W)
**Responsibilities**
- Maintain current folder state for the MP3 module
- Provide method: `requestFolder(folder)`
- Implement the command sequence on folder change:

Sequence:
1) mute volume
2) next/prev folder commands until desired folder reached
3) random-in-folder command
4) play command
5) volume max command

**Timing**
- Inter-command gaps likely required; avoid long blocking delays.
- Implement as a small state machine / command queue so loop remains responsive.

**Assumptions**
- “Random in folder” starts at Track 1, and each folder contains a blank Track 1.

### 3.4 BtController (BT210)
**Responsibilities**
- Optional initial configuration routine (for module replacement scenarios)
- Not required for normal runtime operation (settings persist in module)
- Module may emit 2 messages on pause/play, but no continuous chatter.

**Serial Strategy (important)**
- During normal runtime, do not rely on BT serial reception.
- If BT setup is performed, do it:
  - at boot before starting heavy LED animations, OR
  - in a user-triggered “BT config mode” (future option).

### 3.5 DisplayController (Matrix + Dial PWM)
Split into two layers:

#### A) ThemeManager
**Responsibilities**
- Determine current `theme` from `displayMode`, `sourceMode`, `folder`
- Detect theme changes and reset pattern state when theme changes

#### B) MatrixRenderer
**Responsibilities**
- Drive WS2812B 8x32 on D7
- Render animations non-blocking via `tick(nowMs)` pattern updates
- Enforce rule: **single color per 8-LED column**

**Patterns**
- PARTY: rainbow marble style animation (continuous)
- FIRE: fire simulation (column-based)
- XMAS: half red/green pulsing
- EERIE: slow green/blue pulse

#### C) DialLightController
**Responsibilities**
- Drive analog dial strip via PWM on D6
- Behaviors:
  - DISPLAY_OFF: solid ON
  - THEME_EERIE: breathing/pulse synced to matrix pulse phase
  - Other themes: fixed brightness (TBD)

---

## 4. Main Loop Scheduling (Non-Blocking)

### 4.1 Proposed Loop Outline

1) `InputManager.update(now)`
2) `FolderSelector.update(now)` (if MP3 selected or always — TBD)
3) Compute `effectiveTheme` from current state
4) If folder changed and MP3 is selected:
   - `Mp3Controller.requestFolder(newFolder)`
5) `Mp3Controller.update(now)` (runs queued commands)
6) `DisplayController.update(now)`:
   - If DISPLAY_OFF: matrix off, dial solid on
   - Else: `MatrixRenderer.tick(now)` and `DialLightController.tick(now)`

### 4.2 Timing Budgets (initial targets)
- Input scan: every loop
- Folder timing measure: <= every 50–100ms (TBD)
- Matrix tick: ~20–50 FPS equivalent (every 20–50ms)
- MP3 command steps: spread over time; do not block > ~10ms chunks

---

## 5. Serial / Resource Considerations (Nano)

### 5.1 Two SoftwareSerial Devices
- MP3 module uses SoftSerial pins (12,11) at 9600.
- BT module uses SoftSerial pins (9,10); setup-only; avoid runtime reception.

**Rule**
- Only one SoftwareSerial should be “listening” at a time.
- Prefer not to listen to BT during normal operation.

### 5.2 WS2812 and Interrupt Sensitivity
- WS2812 updates may disable interrupts briefly.
- This can corrupt SoftwareSerial reception if receiving during LED updates.
- Mitigation:
  - avoid relying on BT reception
  - keep MP3 receive use minimal (prefer command-only)
  - if any receive is required, time it outside LED update windows (advanced; likely unnecessary)

---

## 6. File/Code Organization (Arduino IDE-friendly)

Suggested sketch folder layout:

VintageRadioFinal/
- VintageRadioFinal.ino  (setup/loop; orchestration)
- config.h               (pin defs, thresholds, tunables)
- InputManager.h/.cpp
- FolderSelector.h/.cpp
- Mp3Controller.h/.cpp
- BtController.h/.cpp    (optional; setup routine)
- DisplayController.h/.cpp
- MatrixRenderer.h/.cpp
- DialLightController.h/.cpp
- Patterns_*.h/.cpp      (Party/Fire/Xmas/Eerie)

---

## 7. Configuration (config.h)

Include:
- Pin assignments (D8, D2, D3/4/5, D6, D7, SoftSerial pins)
- Debounce/stability times
- Folder timing thresholds (calibrated)
- Dial brightness values (normal, solid override)
- MP3 volume values (mute/max)
- Theme tick rates / animation parameters

---

## 8. Acceptance Hooks (for testing)
Provide compile-time or runtime debug flags:
- `#define DEBUG_SERIAL 1`
- `#define DEBUG_FOLDER 1`
- `#define DEBUG_MP3 1`

When enabled, print:
- sourceMode, displayMode, folder, theme changes
- MP3 command steps
- capacitor timing readings (for calibration)

---

## 9. Future Extension Points
- DISPLAY_RESERVED (D4): add alternative theme sets later
- ESP32 migration path if Nano resources are exceeded:
  - move to hardware UARTs for modules
  - higher animation complexity possible