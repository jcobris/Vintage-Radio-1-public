
# Vintage Radio Retrofit — Conversation Restart Prompt

We are continuing work on my **Vintage Radio Retrofit** project.

This is an Arduino-based hardware/software project that is already **working and stable**.
The goal of these conversations is to incrementally improve, document, or refine the system
without breaking existing behaviour.

I will paste all relevant source files and documentation into the conversation.
Do NOT attempt to browse GitHub or access external repositories.
Work only from what I provide in the chat.

---

## Project Overview (Context)

- **Owner:** Jeff Cornwell
- **Target MCU:** Arduino Nano (ATmega328P)
- **IDE:** Arduino IDE
- **Status:** Fully functional, long‑running stable build

### Core Features
- Bluetooth audio playback (BT210 module)
- MP3 playback from SD card (DY‑SV5W module)
- Physical audio source selection (2‑pole switch)
- Source detection via digital input
- MP3 folder selection via tuning capacitor timing (RC measurement)
- WS2812B 8×32 LED matrix with themed animations
- PWM dial/tuner LED strip
- Additional WS2812B LED strip
- 3‑position display mode switch
- MP3 next‑track pushbutton

---

## Hardware Summary (Fixed / Verified)

- **Bluetooth (BT210):**
  - SoftwareSerial @ 57600
  - Pins: D9 (TX), D10 (RX)
- **MP3 (DY‑SV5W):**
  - SoftwareSerial @ 9600
  - Pins: D11 (TX), D12 (RX)
- **Source detect:** D2 (LOW = MP3, HIGH = Bluetooth, INPUT_PULLUP)
- **Display mode switch:**  
  - D3 = Normal  
  - D4 = Reserved (future)  
  - D5 = Matrix OFF, dial solid ON
- **Tuning input:** D8 (RC timing)
- **Next‑track button:** D13 (active‑low)
- **LED matrix:** D7 (WS2812B 8×32)
- **Dial/tuner LED strip:** D6 (PWM)
- **Additional LED strip:** A0 (WS2812B)

Pin conflicts are enforced via compile‑time guardrails in `Config.h`.

---

## Software & Coding Constraints (IMPORTANT)

- Arduino Nano memory is limited
- Avoid `String` and dynamic allocation
- Prefer compile‑time configuration
- Debug output must be:
  - Predictable
  - Compile‑time controlled
  - Silent by default
- Serial debug baud: **115200**
- Serial is initialised **once** in the main `.ino`
- Only one SoftwareSerial should be active/listening at a time

---

## Working Style & Expectations

- **One step at a time**
- Do NOT redesign the entire project unless explicitly requested
- Keep behaviour as close as possible to what already works
- If changes are suggested:
  - Explain *why*
  - Keep them minimal
- If files are changed:
  - Return **FULL replacement contents**
  - Specify exact file paths
- If a new file is needed:
  - Provide full content
  - State where it should live

Do not include AI‑specific instructions or meta commentary in project files
unless explicitly asked.

---

## Debug Philosophy

- Global debug master switch exists (`DEBUG`)
- Individual subsystems have explicit compile‑time toggles
- Debug should be useful for:
  - Boot
  - Mode changes
  - Folder changes
  - MP3 command flow
- Avoid spam unless explicitly enabled

---

## CURRENT TASK (Update This Section Each Time)

> Describe the specific task you want to work on *in this session*.
> Examples:
> - Refine LED matrix animations for eerie theme
> - Reduce PWM noise on dial LEDs
> - Update documentation to reflect wiring changes
> - Review a specific module for cleanup

**Current task:**
