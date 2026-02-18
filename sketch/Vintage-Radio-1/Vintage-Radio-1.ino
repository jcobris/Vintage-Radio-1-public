
/* Vintage-Radio-1.ino (main conductor)
   Serial: 115200 (set once here)

   Source detect (Config::PIN_SOURCE_DETECT):
     - LOW  -> MP3 selected
     - HIGH -> Bluetooth selected (INPUT_PULLUP)

   Bluetooth (BT201):
     - SoftwareSerial at 57600 on Config::PIN_BT_RX/PIN_BT_TX
     - Sends AT init commands at boot
     - Sleeps UART by default to avoid SoftwareSerial contention with MP3

   MP3 (DY-SV5W):
     - SoftwareSerial at 9600 on Config::PIN_MP3_RX/PIN_MP3_TX
     - Runs ONLY when MP3 source selected
     - Folder selection is owned by this .ino and pushed via MP3::setDesiredFolder()

   Folder numbering (STANDARDIZED: Option A):
     - 1..4  = real folders 1..4 (Folder 1 = old "00", Folder 4 = old "03")
     - 99    = gap/mute
     - 255   = FAULT (we map this to 99 here)

   Display:
     - LED Matrix (LedMatrix): patterns by folder (1..4) and OFF at 99
     - Tuner LED string (DisplayLED):
         * folders 1–3: solid brightness 60
         * folder 4: sine pulse between 30..255 at LedMatrix::SPOOKY_PULSE_BPM
           and on entry to folder 4 it starts at 60 once, then sine takes over

   Display Mode switch (3 poles to GND):
     - D3 LOW: Normal behaviour (as above)
     - D4 LOW: Alt theme (for now behaves same as normal)
     - D5 LOW: Matrix OFF, tuner LED string solid for all 4 folders
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

// Set to 1 when tuner RC input is connected and calibrated
#define TUNER_CONNECTED 0

// Debounce for the physical source switch on D2 (helps on breadboard/switch bounce)
constexpr uint16_t SOURCE_DEBOUNCE_MS = 50;

// When tuner is not connected, force a default folder so MP3 can still play.
constexpr uint8_t DEFAULT_FOLDER_WHEN_NO_TUNER = 1;

// Shared state
static uint8_t g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;

// Adaptive tuning schedule
static unsigned long g_lastTuneMs = 0;
constexpr uint16_t   TUNE_FAST_MS = 300;  // when folder == 4
constexpr uint16_t   TUNE_SLOW_MS = 600;  // when folder != 4

// Bluetooth module (pins from Config.h)
static BluetoothModule btModule(Config::PIN_BT_RX, Config::PIN_BT_TX);

// Source mode
enum class SourceMode : uint8_t { MP3, Bluetooth };
static SourceMode g_lastMode = SourceMode::Bluetooth;

// Display mode switch states
enum class DisplayMode : uint8_t { Normal, Alt, MatrixOff };
static DisplayMode g_lastDisplayMode = DisplayMode::Normal;

// -------- Source detect helpers --------
static SourceMode readSourceModeRaw() {
  // LOW = MP3 selected, HIGH = Bluetooth selected (INPUT_PULLUP)
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

// -------- Display mode switch helper --------
// If your switch guarantees exactly one pin grounded, this will always return one mode.
// Priority is only relevant for fault cases (multiple LOW).
static DisplayMode readDisplayMode() {
  const bool offSelected    = (digitalRead(Config::PIN_DISPLAY_MODE_OFF) == LOW);
  const bool normalSelected = (digitalRead(Config::PIN_DISPLAY_MODE_NORMAL) == LOW);
  const bool altSelected    = (digitalRead(Config::PIN_DISPLAY_MODE_ALT) == LOW);

  if (offSelected) return DisplayMode::MatrixOff;
  if (normalSelected) return DisplayMode::Normal;
  if (altSelected) return DisplayMode::Alt;

  // Fallback (should not happen if switch always grounds one pole)
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

// -------- Folder sanitization (single policy point) --------
// Ensures only {1..4, 99} leave this function. Maps 255 -> 99.
static uint8_t sanitizeFolder(uint8_t f) {
  if (f == 255) return 99;                     // FAULT -> gap/mute
  if ((f >= 1 && f <= 4) || (f == 99)) return f;
  return 99;
}

void setup() {
  // Serial for debug (set once here)
  Serial.begin(115200);
  while (!Serial) { /* harmless on some boards */ }
  delay(200);

  // Source detect
  pinMode(Config::PIN_SOURCE_DETECT, INPUT_PULLUP);

  // Display mode switch (3 poles to GND)
  pinMode(Config::PIN_DISPLAY_MODE_NORMAL, INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_ALT, INPUT_PULLUP);
  pinMode(Config::PIN_DISPLAY_MODE_OFF, INPUT_PULLUP);

  // ---- Display init (matrix + tuner LED string) ----
  LedMatrix::begin();
  DisplayLED::begin(Config::PIN_LED_DISPLAY);

  // ---- Bluetooth init (AT config on boot) ----
  btModule.begin(Config::BT_BAUD);
  Serial.println(F("[BOOT] Configuring BT201..."));
  btModule.sendInitialCommands();
  Serial.println(F("[BOOT] BT201 init done."));

#if BT_PASSTHROUGH == 0
  btModule.sleep();
  Serial.println(F("[BOOT] BT201 UART slept (passthrough disabled)."));
#else
  Serial.println(F("[BOOT] BT201 UART active (passthrough enabled)."));
#endif

  // ---- MP3 init ----
  MP3::init();
  Serial.println(F("[BOOT] MP3 init done."));

  // ---- IMPORTANT: Prime the source debouncer at boot ----
  // Without this, the debouncer's initial 'stable' default can log Bluetooth,
  // then flip to MP3 ~50ms later, even when the switch is firmly on MP3.
  (void)readSourceModeDebounced(millis());
  delay(SOURCE_DEBOUNCE_MS + 10);
  g_lastMode = readSourceModeDebounced(millis());

  Serial.print(F("[MODE] Initial source: "));
  Serial.println((g_lastMode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));

  // Capture initial display mode
  g_lastDisplayMode = readDisplayMode();
  Serial.print(F("[DISP] Initial display mode: "));
  Serial.println(displayModeName(g_lastDisplayMode));
}

void loop() {
  const unsigned long now = millis();

  // Determine source mode (debounced)
  const SourceMode mode = readSourceModeDebounced(now);

  // Log source mode changes
  if (mode != g_lastMode) {
    g_lastMode = mode;
    Serial.print(F("[MODE] Source changed to: "));
    Serial.println((mode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));
  }

  // Determine display mode (switch)
  const DisplayMode dispMode = readDisplayMode();
  if (dispMode != g_lastDisplayMode) {
    g_lastDisplayMode = dispMode;
    Serial.print(F("[DISP] Display mode changed to: "));
    Serial.println(displayModeName(dispMode));
  }

  // ------------------------ Folder selection ------------------------
  if (mode == SourceMode::MP3) {

#if TUNER_CONNECTED == 1
    const uint16_t tuneInterval = (g_currentFolder == 4) ? TUNE_FAST_MS : TUNE_SLOW_MS;
    if (now - g_lastTuneMs >= tuneInterval) {
      uint8_t f = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);
      g_lastTuneMs = now;

      f = sanitizeFolder(f);

      // Print only on change
      static uint8_t lastPrintedFolder = 0;
      if (f != lastPrintedFolder) {
        lastPrintedFolder = f;
        Serial.print(F("[TUNE] Folder = "));
        Serial.println(f);
      }

      g_currentFolder = f;
    }
#else
    // Tuner not wired: stable default folder
    g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;
#endif

  } else {
    // Bluetooth selected: leave folder stable for now.
  }

  // ------------------------ Display folder selection ------------------------
  // MP3 mode uses tuned/current folder; Bluetooth uses folder 1 (party) for now.
  const uint8_t displayFolder = (mode == SourceMode::MP3) ? sanitizeFolder(g_currentFolder) : (uint8_t)1;

  // ------------------------ Display update (matrix + LED string) ------------------------
  const bool lightsOn = (dispMode != DisplayMode::MatrixOff);
  LedMatrix::update(displayFolder, lightsOn);

  if (dispMode == DisplayMode::MatrixOff) {
    // Solid for all folders when matrix is off
    DisplayLED::setSolid(60);
  } else {
    static uint8_t lastDisplayFolderSeen = 0;
    const bool folderChanged = (displayFolder != lastDisplayFolderSeen);

    if (folderChanged) {
      lastDisplayFolderSeen = displayFolder;
      if (displayFolder == 4) {
        // Start at 60 on entry to folder 4
        DisplayLED::setSolid(60);
      }
    }

    if (displayFolder >= 1 && displayFolder <= 3) {
      DisplayLED::setSolid(60);
    } else if (displayFolder == 4) {
      DisplayLED::pulseSineTick(
        LedMatrix::SPOOKY_PULSE_BPM,
        30,
        255,
        18
      );
    } else {
      // 99: leave as-is for now
    }
  }

  // ------------------------ MP3 tick (MP3 mode only) ------------------------
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
