
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

// Dial / tuning illumination LED (PWM)
static const uint8_t DIAL_SOLID_NORMAL = 60;
static const uint8_t DIAL_SOLID_ALT    = 30;

// When the matrix is switched OFF, WS2812 load disappears and the 5V rail can rise,
// making the dial LED appear noticeably brighter at the same PWM level.
// Provide a separate solid brightness for "matrix off" to compensate perceptually.
static const uint8_t DIAL_SOLID_MATRIX_OFF_NORMAL = 40; // tune to taste
static const uint8_t DIAL_SOLID_MATRIX_OFF_ALT    = 25; // tune to taste

// Pulse behaviour (folder 4) - shared breath for matrix + dial
static const uint8_t DIAL_PULSE_MIN_NORMAL = 30;
static const uint8_t DIAL_PULSE_MAX_NORMAL = 60;
static const uint8_t DIAL_PULSE_MIN_ALT    = 10;
static const uint8_t DIAL_PULSE_MAX_ALT    = 30;
static const uint8_t DIAL_PULSE_BPM        = 12;

// Flicker behaviour (between stations)
static const uint8_t  DIAL_FLICKER_MIN_NORMAL = 30;
static const uint8_t  DIAL_FLICKER_MAX_NORMAL = 60;
static const uint8_t  DIAL_FLICKER_MIN_ALT    = 10;
static const uint8_t  DIAL_FLICKER_MAX_ALT    = 30;
static const uint16_t DIAL_FLICKER_TICK_MS    = 10;

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
  // 1..4 = valid
  if (f >= 1 && f <= 4) return f;

  // 99 = gap
  if (f == 99) return 99;

  // 255 = fault (treat as gap)
  // anything else also treated as gap
  return 99;
}

// ============================================================
// Setup
// ============================================================
void setup() {
  // Serial debug must be set only once, here:
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

  // NEW: Limit WS2812 power draw to reduce 5V sag that can dim the dial LED under load.
  // This scales LED output only when required to stay under the budget.
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1200);

  DisplayLED::begin(Config::PIN_LED_DISPLAY);

  g_bt.begin(Config::BT_BAUD);
  g_bt.sendInitialCommands();
  g_bt.sleep();

  MP3::init();

  // Initial matrix brightness (single source of truth is this sketch)
  FastLED.setBrightness(MATRIX_BRIGHT_NORMAL);

  DBG_BOOT(F("[BOOT] System ready"));
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
        DBG_SRC(F("Display mode: NORMAL"));
        FastLED.setBrightness(MATRIX_BRIGHT_NORMAL);
        break;

      case DISPLAY_ALT:
        DBG_SRC(F("Display mode: ALTERNATE"));
        FastLED.setBrightness(MATRIX_BRIGHT_ALT);
        break;

      case DISPLAY_OFF:
        DBG_SRC(F("Display mode: MATRIX_OFF"));
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
      DBG_SRC(F("[SRC] Bluetooth"));
    } else {
      g_bt.sleep();
      DBG_SRC(F("[SRC] MP3"));
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

  if ((now - g_nextBtnLastChangeMs) >= NEXT_BTN_DEBOUNCE_MS &&
      g_nextBtnStable != g_nextBtnRaw) {

    g_nextBtnStable = g_nextBtnRaw;

    if (g_nextBtnStable == LOW) {
      if (g_sourceMode == SOURCE_MP3) {
        MP3::nextTrack();
        DBG_MP3(F("MP3: Next track (button)"));
      }
    }
  }

  // ----------------------------------------------------------
  // Folder selection (isolate RC timing from FastLED.show)
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
  // Dial LED + Matrix Folder 4 shared breathing (dimmed in ALT)
  // ----------------------------------------------------------
  if (!lightsOn) {
    // Matrix off: dial forced solid ON (use dedicated brightness to compensate
    // for higher apparent brightness when WS2812 load is removed)
    DisplayLED::setSolid(altMode ? DIAL_SOLID_MATRIX_OFF_ALT : DIAL_SOLID_MATRIX_OFF_NORMAL);
    LedMatrix::setSpookyBreath(0xFF); // release override
    LedStrip::setSpookyBreath(0xFF);  // release override (NEW)
  } else if (g_folder == 99) {
    DisplayLED::flickerRandomTick(
      altMode ? DIAL_FLICKER_MIN_ALT : DIAL_FLICKER_MIN_NORMAL,
      altMode ? DIAL_FLICKER_MAX_ALT : DIAL_FLICKER_MAX_NORMAL,
      DIAL_FLICKER_TICK_MS
    );
    LedMatrix::setSpookyBreath(0xFF); // release override
    LedStrip::setSpookyBreath(0xFF);  // release override (NEW)
  } else if (g_folder == 4) {
    // One shared breath value: matrix + dial (and now strip) are perfectly in sync
    const uint8_t sharedBreath = beatsin8(
      DIAL_PULSE_BPM,
      altMode ? DIAL_PULSE_MIN_ALT : DIAL_PULSE_MIN_NORMAL,
      altMode ? DIAL_PULSE_MAX_ALT : DIAL_PULSE_MAX_NORMAL
    );

    DisplayLED::setSolid(sharedBreath);
    LedMatrix::setSpookyBreath(sharedBreath);
    LedStrip::setSpookyBreath(sharedBreath); // NEW: strip uses exact same shared value
  } else {
    DisplayLED::setSolid(altMode ? DIAL_SOLID_ALT : DIAL_SOLID_NORMAL);
    LedMatrix::setSpookyBreath(0xFF); // release override
    LedStrip::setSpookyBreath(0xFF);  // release override (NEW)
  }

  // ----------------------------------------------------------
  // LEDs: skip LED updates on the RC-measurement iteration
  // Prevent FastLED.show() (inside LedMatrix::update) overlapping RC timing.
  // ----------------------------------------------------------
  if (!didTunePollThisLoop) {
    // Strip update first (no FastLED.show() here)
    LedStrip::update(g_folder, lightsOn);

    // Matrix update (calls FastLED.show())
    LedMatrix::update(g_folder, lightsOn);
  }
}
