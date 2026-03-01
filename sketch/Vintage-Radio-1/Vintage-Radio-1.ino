
#include <Arduino.h>

#include "Config.h"
#include "LedMatrix.h"
#include "DisplayLED.h"
#include "Radio_Tuning.h"
#include "MP3.h"
#include "Bluetooth.h"

// ============================================================
// Display mode (3‑way switch)
// ============================================================
enum DisplayMode {
  DISPLAY_NORMAL,
  DISPLAY_ALT,
  DISPLAY_OFF
};

static DisplayMode g_displayMode     = DISPLAY_NORMAL;
static DisplayMode g_lastDisplayMode = (DisplayMode)255;

// ============================================================
// State
// ============================================================
static uint8_t g_folder = 1;
static BluetoothModule g_bt(Config::PIN_BT_RX, Config::PIN_BT_TX);

// ============================================================
// Helpers
// ============================================================
static DisplayMode readDisplayMode() {
  // INPUT_PULLUP, LOW = selected
  if (digitalRead(Config::PIN_DISPLAY_MODE_OFF) == LOW) {
    return DISPLAY_OFF;
  }
  if (digitalRead(Config::PIN_DISPLAY_MODE_ALT) == LOW) {
    return DISPLAY_ALT;
  }
  return DISPLAY_NORMAL;
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  // Switch inputs
  pinMode(Config::PIN_DISPLAY_MODE_NORMAL, INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_ALT,    INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_OFF,    INPUT_PULLUP);
  pinMode(Config::PIN_SOURCE_DETECT,       INPUT_PULLUP);

  // Subsystems
  LedMatrix::begin();
  DisplayLED::begin(Config::PIN_LED_DISPLAY);

  g_bt.begin(Config::BT_BAUD);
  g_bt.sendInitialCommands();
  g_bt.sleep();

  MP3::init();

  debugln(F("[BOOT] System ready"));
}

// ============================================================
// Loop
// ============================================================
void loop() {

  // ----------------------------------------------------------
  // Display mode handling (ONLY detection + debug)
  // ----------------------------------------------------------
  g_displayMode = readDisplayMode();
  if (g_displayMode != g_lastDisplayMode) {
    g_lastDisplayMode = g_displayMode;

    switch (g_displayMode) {
      case DISPLAY_NORMAL:
        debugln(F("Display mode: NORMAL"));
        break;
      case DISPLAY_ALT:
        debugln(F("Display mode: ALTERNATE"));
        break;
      case DISPLAY_OFF:
        debugln(F("Display mode: MATRIX_OFF"));
        break;
    }
  }

  // ----------------------------------------------------------
  // Folder selection (unchanged)
  // ----------------------------------------------------------
  g_folder = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);

  // ----------------------------------------------------------
  // Source handling (unchanged)
  // ----------------------------------------------------------
  if (digitalRead(Config::PIN_SOURCE_DETECT) == LOW) {
    // MP3 selected
    g_bt.sleep();
    MP3::setDesiredFolder(g_folder);
    MP3::tick();
  } else {
    // Bluetooth selected
    g_bt.wake();
    g_folder = 1; // Party display for BT
  }

  // ----------------------------------------------------------
  // Display outputs (UNCHANGED APIs)
  // ----------------------------------------------------------
  const bool lightsOn = (g_displayMode != DISPLAY_OFF);

  // DisplayLED behaviour is handled internally as before
  // (no update() call here)

  // Matrix call EXACTLY matches existing API
  LedMatrix::update(g_folder, lightsOn);
}

