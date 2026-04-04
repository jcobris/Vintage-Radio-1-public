
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
static const uint8_t DIAL_SOLID_MATRIX_OFF_NORMAL = 30; // tune to taste
static const uint8_t DIAL_SOLID_MATRIX_OFF_ALT    = 25; // tune to taste

// ------------------------------------------------------------
// Folder 4 breathing (shared for matrix/strip)
// ------------------------------------------------------------
static const uint8_t DIAL_PULSE_MIN_NORMAL = 8;
static const uint8_t DIAL_PULSE_MAX_NORMAL = 55;
static const uint8_t DIAL_PULSE_MIN_ALT    = 6;
static const uint8_t DIAL_PULSE_MAX_ALT    = 40;
static const uint8_t DIAL_PULSE_BPM        = 9;

// ------------------------------------------------------------
// Folder 4 dial-only breathing window (prevents "off" without clamping)
// ------------------------------------------------------------
// These only affect the D3 dial LED. Matrix/strip are unchanged.
static const uint8_t DIAL_F4_DIAL_MIN_NORMAL = 32;
static const uint8_t DIAL_F4_DIAL_MAX_NORMAL = 55;
static const uint8_t DIAL_F4_DIAL_MIN_ALT    = 32;
static const uint8_t DIAL_F4_DIAL_MAX_ALT    = 55;

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

// Folder 4 shared breath smoothing (Q8.8 fixed point)
static uint16_t g_spookyBreathQ8_8 = 0;

// Dial dithering accumulator (16-bit fractional error diffusion)
static uint16_t g_dialDitherErr16 = 0;

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

// Dial-only smooth breathing output (same BPM as matrix/strip, but mapped to a safe PWM window)
// Uses high-resolution temporal dithering to reduce visible stepping at low PWM levels.
static inline uint8_t dialBreathMapped(bool altMode) {
  const uint8_t dMin = altMode ? DIAL_F4_DIAL_MIN_ALT : DIAL_F4_DIAL_MIN_NORMAL;
  const uint8_t dMax = altMode ? DIAL_F4_DIAL_MAX_ALT : DIAL_F4_DIAL_MAX_NORMAL;

  if (dMax <= dMin) return dMin;

  const uint8_t range = (uint8_t)(dMax - dMin);

  // 16-bit sine, phase-locked to millis() at DIAL_PULSE_BPM
  const uint16_t wave16 = beatsin16(DIAL_PULSE_BPM, 0, 65535);

  // Convert to 0..255 shape (still for easing), but KEEP the full 16-bit wave for fraction.
  uint8_t shape8 = (uint8_t)(wave16 >> 8);
  shape8 = ease8InOutQuad(shape8);

  // Re-expand eased shape back to 16-bit so fractional resolution exists even at low end.
  // This keeps the curve smooth while allowing much finer dithering steps.
  const uint16_t eased16 = (uint16_t)shape8 << 8; // 0..65535 in steps of 256 (still better than before after mapping)

  // Map eased16 (0..65535) into dMin..dMax using 16-bit math:
  // value16 = dMin*65535 + eased16*range
  // out = floor(value16 / 65535), frac = remainder
  const uint32_t scaled32 = (uint32_t)eased16 * (uint32_t)range; // 0..(65535*range)

  // Integer part (0..range) and 16-bit fractional part (0..65534)
  const uint16_t baseAdd = (uint16_t)(scaled32 >> 16);           // ~0..range
  const uint16_t frac16  = (uint16_t)(scaled32 & 0xFFFF);        // fractional

  uint8_t out = (uint8_t)(dMin + (uint8_t)baseAdd);

  // High-res temporal dithering: accumulate 16-bit fraction.
  // If it overflows, bump output by 1 (never exceed dMax).
  g_dialDitherErr16 = (uint16_t)(g_dialDitherErr16 + frac16);
  if (g_dialDitherErr16 < frac16) { // overflow occurred
    if (out < dMax) out++;
  }

  return out;
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
  // Next-track button debounce
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
  // Folder selection (RC timing protected)
  // ----------------------------------------------------------
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

  // ----------------------------------------------------------
  // MP3 control
  // ----------------------------------------------------------
  if (g_sourceMode == SOURCE_MP3) {
    MP3::setDesiredFolder(g_folder);
    MP3::tick();
  }

  // ----------------------------------------------------------
  // Dial LED + shared Folder 4 breath
  // ----------------------------------------------------------
  if (!lightsOn) {
    DisplayLED::setSolid(altMode ? DIAL_SOLID_MATRIX_OFF_ALT : DIAL_SOLID_MATRIX_OFF_NORMAL);
    LedMatrix::setSpookyBreath(0xFF);
    LedStrip::setSpookyBreath(0xFF);
    resetSpookyBreathSmoother();
    g_dialDitherErr16 = 0;
  } else if (g_folder == 99) {
    DisplayLED::flickerRandomTick(
      altMode ? DIAL_FLICKER_MIN_ALT : DIAL_FLICKER_MIN_NORMAL,
      altMode ? DIAL_FLICKER_MAX_ALT : DIAL_FLICKER_MAX_NORMAL,
      DIAL_FLICKER_TICK_MS
    );
    LedMatrix::setSpookyBreath(0xFF);
    LedStrip::setSpookyBreath(0xFF);
    resetSpookyBreathSmoother();
    g_dialDitherErr16 = 0;
  } else if (g_folder == 4) {
    // Matrix/strip keep existing shared breath (unchanged)
    const uint8_t rawBreath = beatsin8(
      DIAL_PULSE_BPM,
      altMode ? DIAL_PULSE_MIN_ALT : DIAL_PULSE_MIN_NORMAL,
      altMode ? DIAL_PULSE_MAX_ALT : DIAL_PULSE_MAX_NORMAL
    );

    const uint8_t sharedBreath = smoothSpookyBreath(rawBreath);

    // Dial uses mapped + high-res dithered sine (no clamp, no flat dwell), same BPM phase
    const uint8_t dialOut = dialBreathMapped(altMode);

    DisplayLED::setSolid(dialOut);
    LedMatrix::setSpookyBreath(sharedBreath);
    LedStrip::setSpookyBreath(sharedBreath);
  } else {
    DisplayLED::setSolid(altMode ? DIAL_SOLID_ALT : DIAL_SOLID_NORMAL);
    LedMatrix::setSpookyBreath(0xFF);
    LedStrip::setSpookyBreath(0xFF);
    resetSpookyBreathSmoother();
    g_dialDitherErr16 = 0;
  }

  // ----------------------------------------------------------
  // LEDs: skip updates on the RC-measurement iteration
  // ----------------------------------------------------------
  if (!didTunePollThisLoop) {
    LedStrip::update(g_folder, lightsOn);
    LedMatrix::update(g_folder, lightsOn);
  }
}
