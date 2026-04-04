
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
static const uint8_t DIAL_SOLID_NORMAL = 40;
static const uint8_t DIAL_SOLID_ALT    = 30;

// When the matrix is switched OFF, WS2812 load disappears and the 5V rail can rise,
// making the dial LED appear noticeably brighter at the same PWM level.
// Provide a separate solid brightness for "matrix off" to compensate perceptually.
static const uint8_t DIAL_SOLID_MATRIX_OFF_NORMAL = 30; // tune to taste
static const uint8_t DIAL_SOLID_MATRIX_OFF_ALT    = 25; // tune to taste

// Pulse behaviour (folder 4) - shared breath for matrix + dial
// (These drive matrix/strip. Dial can have an additional floor clamp below.)
static const uint8_t DIAL_PULSE_MIN_NORMAL = 8;
static const uint8_t DIAL_PULSE_MAX_NORMAL = 55;
static const uint8_t DIAL_PULSE_MIN_ALT    = 6;
static const uint8_t DIAL_PULSE_MAX_ALT    = 40;
static const uint8_t DIAL_PULSE_BPM        = 9;   // slowed earlier

// NEW: Dial-only floor clamp for Folder 4 (does not affect matrix/strip)
static const uint8_t DIAL_F4_MIN_FLOOR_NORMAL = 32; // raise if dial still dips too low
static const uint8_t DIAL_F4_MIN_FLOOR_ALT    = 32; // raise if dial still dips too low

// Flicker behaviour (between stations)
static const uint8_t  DIAL_FLICKER_MIN_NORMAL = 30;
static const uint8_t  DIAL_FLICKER_MAX_NORMAL = 40;
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

// Folder 4 breath smoothing (Q8.8 fixed point)
static uint16_t g_spookyBreathQ8_8 = 0;

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

static inline void resetSpookyBreathSmoother() {
  g_spookyBreathQ8_8 = 0;
}

static inline uint8_t smoothSpookyBreath(uint8_t target) {
  const uint16_t targetQ = (uint16_t)target << 8;
  if (g_spookyBreathQ8_8 == 0) g_spookyBreathQ8_8 = targetQ;
  const int16_t diff = (int16_t)(targetQ - g_spookyBreathQ8_8);
  g_spookyBreathQ8_8 = (uint16_t)(g_spookyBreathQ8_8 + (diff >> 2));
  return (uint8_t)(g_spookyBreathQ8_8 >> 8);
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

  LedMatrix::begin();
  LedStrip::begin();

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1200);

  DisplayLED::begin(Config::PIN_LED_DISPLAY);

  g_bt.begin(Config::BT_BAUD);
  g_bt.sendInitialCommands();
  g_bt.sleep();

  MP3::init();

  FastLED.setBrightness(MATRIX_BRIGHT_NORMAL);

  DBG_BOOT(F("[BOOT] System ready"));
}

// ============================================================
// Loop
// ============================================================
void loop() {
  const uint32_t now = millis();

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

  bool didTunePollThisLoop = false;

  if (g_sourceMode == SOURCE_MP3) {
    if (now - g_lastTunePollMs >= TUNE_POLL_MS) {
      g_lastTunePollMs = now;
      didTunePollThisLoop = true;
      g_folder = sanitizeFolder(RadioTuning::getFolder(Config::PIN_TUNING_INPUT));
    }
  } else {
    g_folder = 1;
  }

  if (g_sourceMode == SOURCE_MP3) {
    MP3::setDesiredFolder(g_folder);
    MP3::tick();
  }

  if (!lightsOn) {
    DisplayLED::setSolid(altMode ? DIAL_SOLID_MATRIX_OFF_ALT : DIAL_SOLID_MATRIX_OFF_NORMAL);
    LedMatrix::setSpookyBreath(0xFF);
    LedStrip::setSpookyBreath(0xFF);
    resetSpookyBreathSmoother();
  } else if (g_folder == 99) {
    DisplayLED::flickerRandomTick(
      altMode ? DIAL_FLICKER_MIN_ALT : DIAL_FLICKER_MIN_NORMAL,
      altMode ? DIAL_FLICKER_MAX_ALT : DIAL_FLICKER_MAX_NORMAL,
      DIAL_FLICKER_TICK_MS
    );
    LedMatrix::setSpookyBreath(0xFF);
    LedStrip::setSpookyBreath(0xFF);
    resetSpookyBreathSmoother();
  } else if (g_folder == 4) {
    const uint8_t rawBreath = beatsin8(
      DIAL_PULSE_BPM,
      altMode ? DIAL_PULSE_MIN_ALT : DIAL_PULSE_MIN_NORMAL,
      altMode ? DIAL_PULSE_MAX_ALT : DIAL_PULSE_MAX_NORMAL
    );

    const uint8_t sharedBreath = smoothSpookyBreath(rawBreath);

    // Dial-only clamp (matrix/strip keep the exact sharedBreath low that you like)
    uint8_t dialBreath = sharedBreath;
    const uint8_t floorVal = altMode ? DIAL_F4_MIN_FLOOR_ALT : DIAL_F4_MIN_FLOOR_NORMAL;
    if (dialBreath < floorVal) dialBreath = floorVal;

    DisplayLED::setSolid(dialBreath);
    LedMatrix::setSpookyBreath(sharedBreath);
    LedStrip::setSpookyBreath(sharedBreath);
  } else {
    DisplayLED::setSolid(altMode ? DIAL_SOLID_ALT : DIAL_SOLID_NORMAL);
    LedMatrix::setSpookyBreath(0xFF);
    LedStrip::setSpookyBreath(0xFF);
    resetSpookyBreathSmoother();
  }

  // Skip LED updates on RC-measurement iteration (RC timing protection)
  if (!didTunePollThisLoop) {
    LedStrip::update(g_folder, lightsOn);
    LedMatrix::update(g_folder, lightsOn);
  }
}
