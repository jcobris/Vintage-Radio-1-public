
/*
  Vintage-Radio-1.ino (main conductor)
  Serial: 115200 (set once here)

  Hardware / pin roles (see Config.h for pin numbers):
  - Source detect: LOW=MP3, HIGH=Bluetooth (INPUT_PULLUP)
  - Bluetooth (BT201): SoftwareSerial 57600, UART slept unless passthrough enabled
  - MP3 (DY-SV5W): SoftwareSerial 9600, folder control via MP3::setDesiredFolder()
  - Tuning input: RC timing read on D8
  - Matrix: WS2812B data on D7
  - Dial/tuner LED string: PWM on D6

  Folder numbering (Option A):
  - 1..4  = SD folders 1..4
  - 99    = "gap" (visual only in this build; audio remains stable)
  - 255   = fault (treated as 99 for safety before calling MP3)

  Display mode switch (3 poles to GND, INPUT_PULLUP):
  - D3 LOW: Normal
  - D4 LOW: Alt (same as normal for now)
  - D5 LOW: Matrix OFF, dial LED solid

  Between-stations visual effect:
  - Matrix continues showing the committed folder theme (no blanking)
  - Dial LED flickers whenever the instantaneous tuner classification is 99
    (this does not affect MP3/audio)
*/

#include <Arduino.h>

#include "Config.h"
#include "Radio_Tuning.h"
#include "Bluetooth.h"
#include "MP3.h"

// Display modules
#include "LedMatrix.h"
#include "DisplayLED.h"

// -------------------- Compile-time controls --------------------
#define BT_PASSTHROUGH 0
#define TUNER_CONNECTED 1

constexpr uint16_t SOURCE_DEBOUNCE_MS = 50;
constexpr uint8_t  DEFAULT_FOLDER_WHEN_NO_TUNER = 1;

static uint8_t g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;

static unsigned long g_lastTuneMs = 0;
constexpr uint16_t   TUNE_FAST_MS = 300;  // when folder == 4
constexpr uint16_t   TUNE_SLOW_MS = 600;  // when folder != 4

// Latest instantaneous tuning classification from last measurement (1..4, 99, 255)
static uint8_t g_instantTuneClass = 99;

static BluetoothModule btModule(Config::PIN_BT_RX, Config::PIN_BT_TX);

enum class SourceMode : uint8_t { MP3, Bluetooth };
static SourceMode g_lastMode = SourceMode::Bluetooth;

enum class DisplayMode : uint8_t { Normal, Alt, MatrixOff };
static DisplayMode g_lastDisplayMode = DisplayMode::Normal;

static SourceMode readSourceModeRaw() {
  const bool mp3Selected = (digitalRead(Config::PIN_SOURCE_DETECT) == LOW);
  return mp3Selected ? SourceMode::MP3 : SourceMode::Bluetooth;
}

static SourceMode readSourceModeDebounced(unsigned long nowMs) {
  static SourceMode lastRaw = SourceMode::Bluetooth;
  static SourceMode stable  = SourceMode::Bluetooth;
  static unsigned long lastFlipMs = 0;

  SourceMode raw = readSourceModeRaw();
  if (raw != lastRaw) {
    lastRaw = raw;
    lastFlipMs = nowMs;
  }

  if ((nowMs - lastFlipMs) >= SOURCE_DEBOUNCE_MS && stable != lastRaw) {
    stable = lastRaw;
  }

  return stable;
}

static DisplayMode readDisplayMode() {
  const bool offSelected    = (digitalRead(Config::PIN_DISPLAY_MODE_OFF) == LOW);
  const bool normalSelected = (digitalRead(Config::PIN_DISPLAY_MODE_NORMAL) == LOW);
  const bool altSelected    = (digitalRead(Config::PIN_DISPLAY_MODE_ALT) == LOW);

  if (offSelected) return DisplayMode::MatrixOff;
  if (normalSelected) return DisplayMode::Normal;
  if (altSelected) return DisplayMode::Alt;
  return DisplayMode::Normal;
}

static const __FlashStringHelper* displayModeName(DisplayMode m) {
  switch (m) {
    case DisplayMode::Normal:    return F("NORMAL");
    case DisplayMode::Alt:       return F("ALT");
    case DisplayMode::MatrixOff: return F("MATRIX_OFF");
    default:                     return F("UNKNOWN");
  }
}

static uint8_t sanitizeFolder(uint8_t f) {
  if (f == 255) return 99;
  if ((f >= 1 && f <= 4) || (f == 99)) return f;
  return 99;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* harmless on some boards */ }
  delay(200);

  pinMode(Config::PIN_SOURCE_DETECT, INPUT_PULLUP);

  pinMode(Config::PIN_DISPLAY_MODE_NORMAL, INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_ALT, INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_OFF, INPUT_PULLUP);

  LedMatrix::begin();
  DisplayLED::begin(Config::PIN_LED_DISPLAY);

  btModule.begin(Config::BT_BAUD);

  if (DEBUG == 1) {
    Serial.println(F("[BOOT] Configuring BT201..."));
  }
  btModule.sendInitialCommands();
  if (DEBUG == 1) {
    Serial.println(F("[BOOT] BT201 init done."));
  }

#if BT_PASSTHROUGH == 0
  btModule.sleep();
  if (DEBUG == 1) {
    Serial.println(F("[BOOT] BT201 UART slept (passthrough disabled)."));
  }
#else
  if (DEBUG == 1) {
    Serial.println(F("[BOOT] BT201 UART active (passthrough enabled)."));
  }
#endif

  MP3::init();
  if (DEBUG == 1) {
    Serial.println(F("[BOOT] MP3 init done."));
  }

  (void)readSourceModeDebounced(millis());
  delay(SOURCE_DEBOUNCE_MS + 10);
  g_lastMode = readSourceModeDebounced(millis());

  if (DEBUG == 1) {
    Serial.print(F("[MODE] Initial source: "));
    Serial.println((g_lastMode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));

    g_lastDisplayMode = readDisplayMode();
    Serial.print(F("[DISP] Initial display mode: "));
    Serial.println(displayModeName(g_lastDisplayMode));
  } else {
    g_lastDisplayMode = readDisplayMode();
  }
}

void loop() {
  const unsigned long now = millis();

  const SourceMode mode = readSourceModeDebounced(now);
  if (mode != g_lastMode) {
    g_lastMode = mode;
    if (DEBUG == 1) {
      Serial.print(F("[MODE] Source changed to: "));
      Serial.println((mode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));
    }
  }

  const DisplayMode dispMode = readDisplayMode();
  if (dispMode != g_lastDisplayMode) {
    g_lastDisplayMode = dispMode;
    if (DEBUG == 1) {
      Serial.print(F("[DISP] Display mode changed to: "));
      Serial.println(displayModeName(dispMode));
    }
  }

  if (mode == SourceMode::MP3) {
#if TUNER_CONNECTED == 1
    const uint16_t tuneInterval = (g_currentFolder == 4) ? TUNE_FAST_MS : TUNE_SLOW_MS;
    if (now - g_lastTuneMs >= tuneInterval) {
      uint8_t f = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);
      g_instantTuneClass = RadioTuning::getInstantClass();
      g_lastTuneMs = now;

      f = sanitizeFolder(f);

      // Only print tuner changes when DEBUG enabled
      if (DEBUG == 1) {
        static uint8_t lastPrintedFolder = 0;
        if (f != lastPrintedFolder) {
          lastPrintedFolder = f;
          Serial.print(F("[TUNE] Folder = "));
          Serial.println(f);
        }
      }

      g_currentFolder = f;
    }
#else
    g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;
    g_instantTuneClass = 99;
#endif
  } else {
    // Not in MP3 mode: no visual gap effect
    g_instantTuneClass = 0;
  }

  // Visual gap triggers on every instantaneous 99 sample
  const bool visualGap = (mode == SourceMode::MP3) && (g_instantTuneClass == 99);

  // Matrix always uses committed folder theme (no blanking on instant gap)
  const uint8_t displayFolder = (mode == SourceMode::MP3)
      ? sanitizeFolder(g_currentFolder)
      : (uint8_t)1;

  const bool lightsOn = (dispMode != DisplayMode::MatrixOff);
  LedMatrix::update(displayFolder, lightsOn);

  // Dial LED behaviour
  if (dispMode == DisplayMode::MatrixOff) {
    DisplayLED::setSolid(Config::DISPLAY_SOLID_BRIGHT);
  } else {
    if (visualGap) {
      // Between-stations flicker (visual only)
      DisplayLED::flickerRandomTick(10, 80, 35);
    } else {
      if (displayFolder >= 1 && displayFolder <= 3) {
        DisplayLED::setSolid(Config::DISPLAY_SOLID_BRIGHT);
      } else if (displayFolder == 4) {
        DisplayLED::pulseSineTick(
          LedMatrix::SPOOKY_PULSE_BPM,
          Config::DISPLAY_PULSE_MIN,
          Config::DISPLAY_PULSE_MAX,
          Config::DISPLAY_PULSE_TICK_MS
        );
      } else {
        DisplayLED::setSolid(Config::DISPLAY_SOLID_BRIGHT);
      }
    }
  }

  // MP3 control follows committed folder only (audio remains stable)
  if (mode == SourceMode::MP3) {
    const uint8_t safeFolder = sanitizeFolder(g_currentFolder);
    MP3::setDesiredFolder(safeFolder);
    MP3::tick();
  } else {
#if BT_PASSTHROUGH == 1
    btModule.passthrough();
#endif
  }
}
