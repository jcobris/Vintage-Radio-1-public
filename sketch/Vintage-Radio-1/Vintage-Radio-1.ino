
/* Vintage-Radio-1.ino (main conductor)
   Serial: 115200 (set once here)

   Source detect (Config::PIN_SOURCE_DETECT):
     - LOW  -> MP3 selected
     - HIGH -> Bluetooth selected (INPUT_PULLUP)  [2](https://github.com/orgs/music-assistant/discussions/459)

   Bluetooth (BT201):
     - SoftwareSerial at 57600 on Config::PIN_BT_RX/PIN_BT_TX
     - Sends AT init commands at boot
     - Sleeps UART by default to avoid SoftwareSerial contention with MP3  [3](https://github.com/jcobris/Vintage-Radio-1-public/security)

   MP3 (DY-SV5W):
     - SoftwareSerial at 9600 on Config::PIN_MP3_RX/PIN_MP3_TX  [1](https://github.com/jcobris/Vintage-Radio-1-public/blob/main/docs/README.md)
     - Runs ONLY when MP3 source selected
     - Folder selection is owned by this .ino and pushed via MP3::setDesiredFolder()

   Folder numbering (STANDARDIZED: Option A):
     - 1..4  = real folders 1..4 (Folder 1 = old "00", Folder 4 = old "03")
     - 99    = gap/mute
     - 255   = FAULT (we map this to 99 here)

   Display (temporary behavior):
     - If folder == 4 -> pulse (this matches old "folder 3" behavior when you used 0..3)
     - Else           -> solid brightness
*/

#include <Arduino.h>

#include "Config.h"
#include "Radio_Tuning.h"
#include "Display_LED.h"
#include "Bluetooth.h"
#include "MP3.h"

// -------------------- Compile-time controls --------------------
#define BT_PASSTHROUGH 0

// Set to 1 when tuner RC input is connected and calibrated
#define TUNER_CONNECTED 0

// Debounce for the physical source switch on D2 (helps on breadboard/switch bounce)
constexpr uint16_t SOURCE_DEBOUNCE_MS = 50;

// When tuner is not connected, force a default folder so MP3 can still play.
// STANDARD: 1..4 now (Option A)
constexpr uint8_t DEFAULT_FOLDER_WHEN_NO_TUNER = 1;

// Shared state
static uint8_t g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;
static uint8_t s_displayBrightness = 60;

// Adaptive tuning schedule
static unsigned long g_lastTuneMs = 0;
constexpr uint16_t   TUNE_FAST_MS = 300;  // when folder == 4 (old "3")
constexpr uint16_t   TUNE_SLOW_MS = 600;  // when folder != 4

// Bluetooth module (pins from Config.h)
static BluetoothModule btModule(Config::PIN_BT_RX, Config::PIN_BT_TX);

// Source mode
enum class SourceMode : uint8_t { MP3, Bluetooth };
static SourceMode g_lastMode = SourceMode::Bluetooth;

// -------- Source detect helpers --------
static SourceMode readSourceModeRaw() {
  // LOW = MP3 selected, HIGH = Bluetooth selected  [2](https://github.com/orgs/music-assistant/discussions/459)
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

void setup() {
  // Serial for debug (set once here)
  Serial.begin(115200);
  while (!Serial) { /* harmless on some boards */ }
  delay(200);

  // Source detect
  pinMode(Config::PIN_SOURCE_DETECT, INPUT_PULLUP);

  // Display init
  pinMode(Config::PIN_LED_DISPLAY, OUTPUT);
  DisplayLED::init(Config::PIN_LED_DISPLAY);

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

  // Capture initial mode
  g_lastMode = readSourceModeDebounced(millis());
  Serial.print(F("[MODE] Initial source: "));
  Serial.println((g_lastMode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));
}

void loop() {
  const unsigned long now = millis();

  // Determine mode (debounced)
  const SourceMode mode = readSourceModeDebounced(now);

  // Log mode changes (predictable)
  if (mode != g_lastMode) {
    g_lastMode = mode;
    Serial.print(F("[MODE] Source changed to: "));
    Serial.println((mode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));
  }

  if (mode == SourceMode::MP3) {
    // 1) Folder selection
#if TUNER_CONNECTED == 1
    const uint16_t tuneInterval = (g_currentFolder == 4) ? TUNE_FAST_MS : TUNE_SLOW_MS;
    if (now - g_lastTuneMs >= tuneInterval) {
      uint8_t f = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);
      g_lastTuneMs = now;

      // Map FAULT (255) -> gap/mute (99) so faults don't become a real folder
      if (f == 255) f = 99;

      // Accept only 1..4 or 99 (anything else -> 99)
      if (!((f >= 1 && f <= 4) || (f == 99))) f = 99;

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

    // 2) Display behavior (temporary)
    // Previously you pulsed when folder==3 under a 0..3 scheme (i.e., the 4th selection).
    // Under the 1..4 scheme, that becomes folder==4.
    if (g_currentFolder == 4) {
      DisplayLED::pulseTick(20);
    } else {
      analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);
    }

    // 3) MP3 runs only when MP3 is selected
    MP3::setDesiredFolder(g_currentFolder);   // expects 1..4 or 99
    MP3::tick();

  } else {
    // Bluetooth selected: keep display solid for now
    analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);

#if BT_PASSTHROUGH == 1
    btModule.passthrough();
#endif
  }
}
