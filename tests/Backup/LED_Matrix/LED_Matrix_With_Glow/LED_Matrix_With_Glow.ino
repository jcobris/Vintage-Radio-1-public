
// LED_Matrix_With_Glow.ino
// Mode-driven matrix controller for vintage radio project.
// Non-blocking. One color per column (8 LEDs per column).
// Folder mapping:
//  - 1: Pop music  -> Party marble rainbow (smooth & relaxed)
//  - 2: Hard Rock  -> Fire (flickery, fiery columns)
//  - 3: Christmas  -> Half red / half green, calm alternating pulse
//  - 4: Spooky 1940s -> Green/blue slow breathing; DISPLAY LEDs pulse via sine
//  - 99: Dead space (matrix off)
//
// Display_LEDs (tuning illumination):
//  - Pin 9 (PWM). Minimal CPU.
//  - Folders 1–3: solid brightness 60.
//  - Folder 4: sine pulse between 30 and 255, synchronized using SPOOKY_PULSE_BPM.
//  - On entry to folder 4, start at brightness 60, then sine takes over.

#include "LedMatrix.h"

// ------------------- Display LED (tuning illumination) -------------------
namespace DisplayLED {
  // Hardware
  static const uint8_t PIN = 9;            // PWM-capable on AVR (Uno/Nano)

  // Brightness policy
  static const uint8_t SOLID_BRIGHT = 60;  // folders 1–3
  static const uint8_t MIN_BRIGHT   = 30;  // sine lower bound
  static const uint8_t MAX_BRIGHT   = 255; // sine upper bound
  static const uint16_t TICK_MS     = 18;  // cadence ~18ms (low CPU, smooth)

  // State
  static uint8_t  s_currentBright   = SOLID_BRIGHT;
  static uint32_t s_lastTickMs      = 0;
  static int      s_lastFolderSeen  = -1000; // force first update

  inline void begin() {
    pinMode(PIN, OUTPUT);
    s_currentBright = SOLID_BRIGHT;
    analogWrite(PIN, s_currentBright);
    s_lastTickMs   = millis();
    s_lastFolderSeen = -1000;
  }

  // Solid write, but only when needed (avoid repeated analogWrite)
  inline void setSolid60IfChanged() {
    if (s_currentBright != SOLID_BRIGHT) {
      s_currentBright = SOLID_BRIGHT;
      analogWrite(PIN, s_currentBright);
    }
  }

  // Sine breathing using FastLED's beatsin8 at SPOOKY_PULSE_BPM
  inline void sinePulseTick() {
    const uint32_t now = millis();
    if (now - s_lastTickMs < TICK_MS) return; // non-blocking throttle
    s_lastTickMs = now;

    // Calculate sine brightness (phase-locked to FastLED beat timing)
    uint8_t pulse = beatsin8(LedMatrix::SPOOKY_PULSE_BPM, MIN_BRIGHT, MAX_BRIGHT);
    if (pulse != s_currentBright) {
      s_currentBright = pulse;
      analogWrite(PIN, s_currentBright);
    }
  }

  // Entry point per loop: choose policy by folder
  inline void update(int g_currentFolder) {
    const bool folderChanged = (g_currentFolder != s_lastFolderSeen);

    // Handle initial write on folder change
    if (folderChanged) {
      s_lastFolderSeen = g_currentFolder;
      switch (g_currentFolder) {
        case 1:
        case 2:
        case 3:
          // Solid 60 on entry
          setSolid60IfChanged();
          break;
        case 4:
          // Start at 60 on entry, then sine takes over on subsequent ticks
          s_currentBright = SOLID_BRIGHT;
          analogWrite(PIN, s_currentBright);
          break;
        case 99:
          // Optional OFF when in dead space (uncomment if desired)
          // s_currentBright = 0;
          // analogWrite(PIN, 0);
          break;
        default:
          // Leave current brightness; no action
          break;
      }
      // After entry handling, return so sine/solid resumes next loop tick
      return;
    }

    // Steady-state per folder
    switch (g_currentFolder) {
      case 1:
      case 2:
      case 3:
        setSolid60IfChanged();
        break;
      case 4:
        sinePulseTick();
        break;
      default:
        // No change for other folders; keep last brightness
        break;
    }
  }
} // namespace DisplayLED

// --- Simulated inputs (replace later with your tuning RC & source selector) ---
int  g_currentFolder = 4;     // 1..4 or 99 (off mode)
bool lights_on_off   = true;  // true = matrix ON, false = OFF

void setup() {
  Serial.begin(115200);
  delay(300); // small one-time startup pause for serial

  LedMatrix::begin();
  DisplayLED::begin();

  Serial.println(F("Vintage radio matrix controller started."));
  Serial.print(F("Initial lights_on_off = "));
  Serial.println(lights_on_off ? F("true") : F("false"));
  Serial.print(F("Initial g_currentFolder = "));
  Serial.println(g_currentFolder);
}

void loop() {
  // Update LEDs based on current folder & on/off flag (non-blocking)
  LedMatrix::update((uint8_t)g_currentFolder, lights_on_off);

  // Update tuning display LEDs (low CPU, non-blocking)
  DisplayLED::update(g_currentFolder);

  // Example auto-cycler (OPTIONAL, comment out for manual testing):
  // static uint32_t t0 = millis();
  // if (millis() - t0 > 10000) {
  //   g_currentFolder = (g_currentFolder == 4 ? 99 : g_currentFolder + 1);
  //   t0 = millis();
  // }
}
