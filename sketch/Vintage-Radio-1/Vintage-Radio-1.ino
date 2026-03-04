
#include <Arduino.h>
#include <FastLED.h>

#include "Config.h"
#include "LedMatrix.h"
#include "LedStrip.h"
#include "DisplayLED.h"
#include "Radio_Tuning.h"
#include "MP3.h"
#include "Bluetooth.h"

// ============================================================
// USER‑TWEAKABLE BRIGHTNESS (ALL IN ONE PLACE)
// ============================================================

// Matrix brightness (FastLED global)
static const uint8_t MATRIX_BRIGHT_NORMAL = 200;
static const uint8_t MATRIX_BRIGHT_ALT    = 100;

// Dial / tuning illumination LED
static const uint8_t DIAL_SOLID_NORMAL = 60;
static const uint8_t DIAL_SOLID_ALT    = 25;

// Pulse behaviour (folder 4)
static const uint8_t DIAL_PULSE_MIN_NORMAL = 30;
static const uint8_t DIAL_PULSE_MAX_NORMAL = 255;
static const uint8_t DIAL_PULSE_MIN_ALT    = 10;
static const uint8_t DIAL_PULSE_MAX_ALT    = 120;
static const uint16_t DIAL_PULSE_TICK_MS   = 18;
static const uint8_t  DIAL_PULSE_BPM       = 12;

// Flicker behaviour (between stations)
static const uint8_t  DIAL_FLICKER_MIN_NORMAL = 30;
static const uint8_t  DIAL_FLICKER_MAX_NORMAL = 160;
static const uint8_t  DIAL_FLICKER_MIN_ALT    = 10;
static const uint8_t  DIAL_FLICKER_MAX_ALT    = 70;
static const uint16_t DIAL_FLICKER_TICK_MS    = 40;

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
// Source mode
// ============================================================
enum SourceMode {
  SOURCE_MP3,
  SOURCE_BT
};

static SourceMode g_sourceMode     = SOURCE_BT;
static SourceMode g_lastSourceMode = (SourceMode)255;

// ============================================================
// State
// ============================================================
static uint8_t  g_folder = 1;
static uint32_t g_lastTunePollMs = 0;
static const uint16_t TUNE_POLL_MS = 120;

static BluetoothModule g_bt(Config::PIN_BT_RX, Config::PIN_BT_TX);

// Next-track button debounce state (D13, active-low)
static const uint16_t NEXT_BTN_DEBOUNCE_MS = 30;
static uint8_t  g_nextBtnRaw = HIGH;
static uint8_t  g_nextBtnStable = HIGH;
static uint32_t g_nextBtnLastChangeMs = 0;

// ============================================================
// Helpers
// ============================================================
static DisplayMode readDisplayMode() {
  if (digitalRead(Config::PIN_DISPLAY_MODE_OFF) == LOW) return DISPLAY_OFF;
  if (digitalRead(Config::PIN_DISPLAY_MODE_ALT) == LOW) return DISPLAY_ALT;
  return DISPLAY_NORMAL;
}

static SourceMode readSourceMode() {
  return (digitalRead(Config::PIN_SOURCE_DETECT) == LOW) ? SOURCE_MP3 : SOURCE_BT;
}

static uint8_t sanitizeFolder(uint8_t f) {
  if (f >= 1 && f <= 4) return f;
  if (f == 99) return 99;
  return 99;
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(Config::PIN_SOURCE_DETECT, INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_NORMAL, INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_ALT,    INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_OFF,    INPUT_PULLUP);

  pinMode(Config::PIN_NEXT_TRACK_BUTTON, INPUT_PULLUP);
  g_nextBtnRaw = digitalRead(Config::PIN_NEXT_TRACK_BUTTON);
  g_nextBtnStable = g_nextBtnRaw;
  g_nextBtnLastChangeMs = millis();

  // FastLED controllers:
  LedMatrix::begin();
  LedStrip::begin();

  DisplayLED::begin(Config::PIN_LED_DISPLAY);

  g_bt.begin(Config::BT_BAUD);
  g_bt.sendInitialCommands();
  g_bt.sleep();

  MP3::init();

  FastLED.setBrightness(MATRIX_BRIGHT_NORMAL);

  debugln(F("[BOOT] System ready"));
}

// ============================================================
// Loop
// ============================================================
void loop() {
  const uint32_t now = millis();

  // ----------------------------------------------------------
  // Display mode
  // ----------------------------------------------------------
  g_displayMode = readDisplayMode();
  if (g_displayMode != g_lastDisplayMode) {
    g_lastDisplayMode = g_displayMode;

    switch (g_displayMode) {
      case DISPLAY_NORMAL:
        debugln(F("Display mode: NORMAL"));
        FastLED.setBrightness(MATRIX_BRIGHT_NORMAL);
        break;

      case DISPLAY_ALT:
        debugln(F("Display mode: ALTERNATE"));
        FastLED.setBrightness(MATRIX_BRIGHT_ALT);
        break;

      case DISPLAY_OFF:
        debugln(F("Display mode: MATRIX_OFF"));
        break;
    }
  }

  const bool altMode  = (g_displayMode == DISPLAY_ALT);
  const bool lightsOn = (g_displayMode != DISPLAY_OFF);

  // ----------------------------------------------------------
  // Source mode
  // ----------------------------------------------------------
  g_sourceMode = readSourceMode();
  if (g_sourceMode != g_lastSourceMode) {
    g_lastSourceMode = g_sourceMode;
    if (g_sourceMode == SOURCE_BT) {
      g_bt.wake();
      debugln(F("[SRC] Bluetooth"));
    } else {
      g_bt.sleep();
      debugln(F("[SRC] MP3"));
    }
  }

  // ----------------------------------------------------------
  // Next-track button (D13, active-low) - debounced, edge-triggered
  // Only active when MP3 source is selected.
  // ----------------------------------------------------------
  const uint8_t btn = digitalRead(Config::PIN_NEXT_TRACK_BUTTON);
  if (btn != g_nextBtnRaw) {
    g_nextBtnRaw = btn;
    g_nextBtnLastChangeMs = now;
  }
  if ((now - g_nextBtnLastChangeMs) >= NEXT_BTN_DEBOUNCE_MS && g_nextBtnStable != g_nextBtnRaw) {
    g_nextBtnStable = g_nextBtnRaw;
    if (g_nextBtnStable == LOW) {
      if (g_sourceMode == SOURCE_MP3) {
        MP3::nextTrack();
        debugln(F("MP3: Next track (button)"));
      }
    }
  }

  // ----------------------------------------------------------
  // Folder selection (Option 1: isolate RC timing from FastLED.show)
  // ----------------------------------------------------------
  bool didTunePollThisLoop = false;

  if (g_sourceMode == SOURCE_MP3) {
    if (now - g_lastTunePollMs >= TUNE_POLL_MS) {
      g_lastTunePollMs = now;
      didTunePollThisLoop = true;

      // Perform RC measurement + commit logic
      g_folder = sanitizeFolder(RadioTuning::getFolder(Config::PIN_TUNING_INPUT));
    }
  } else {
    g_folder = 1;
  }

  // ----------------------------------------------------------
  // MP3 control
  // ----------------------------------------------------------
  if (g_sourceMode == SOURCE_MP3) {
    MP3::setDesiredFolder(g_folder);
    MP3::tick();
  }

  // ----------------------------------------------------------
  // Dial LED (dimmed in ALT)
  // ----------------------------------------------------------
  if (!lightsOn) {
    DisplayLED::setSolid(altMode ? DIAL_SOLID_ALT : DIAL_SOLID_NORMAL);
  } else if (g_folder == 99) {
    DisplayLED::flickerRandomTick(
      altMode ? DIAL_FLICKER_MIN_ALT : DIAL_FLICKER_MIN_NORMAL,
      altMode ? DIAL_FLICKER_MAX_ALT : DIAL_FLICKER_MAX_NORMAL,
      DIAL_FLICKER_TICK_MS
    );
  } else if (g_folder == 4) {
    DisplayLED::pulseSineTick(
      DIAL_PULSE_BPM,
      altMode ? DIAL_PULSE_MIN_ALT : DIAL_PULSE_MIN_NORMAL,
      altMode ? DIAL_PULSE_MAX_ALT : DIAL_PULSE_MAX_NORMAL,
      DIAL_PULSE_TICK_MS
    );
  } else {
    DisplayLED::setSolid(altMode ? DIAL_SOLID_ALT : DIAL_SOLID_NORMAL);
  }

  // ----------------------------------------------------------
  // LEDs (Option 1): skip LED updates on the RC-measurement iteration
  // This prevents FastLED.show() (called inside LedMatrix::update) from
  // overlapping the RC timing measurement window. [1](https://teamtelstra-my.sharepoint.com/personal/jeff_c_cornwell_team_telstra_com/Documents/Microsoft%20Copilot%20Chat%20Files/LedMatrix.cpp)
  // ----------------------------------------------------------
  if (!didTunePollThisLoop) {
    // Strip update first (no FastLED.show() here)
    LedStrip::update(g_folder, lightsOn);

    // Matrix update (calls FastLED.show())
    LedMatrix::update(g_folder, lightsOn); // show happens inside update [1](https://teamtelstra-my.sharepoint.com/personal/jeff_c_cornwell_team_telstra_com/Documents/Microsoft%20Copilot%20Chat%20Files/LedMatrix.cpp)
  }
}
