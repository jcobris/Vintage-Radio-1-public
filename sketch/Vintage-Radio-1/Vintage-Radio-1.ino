
/* Vintage-Radio-1.ino (main)
   - Serial at 115200 (set once here)
   - Source select on D2 (Config::PIN_SOURCE_DETECT):
       * LOW  -> MP3 selected
       * HIGH -> Bluetooth selected (INPUT_PULLUP)
   - Bluetooth:
       * Sends AT init commands on boot
       * By default, Bluetooth SoftwareSerial is put to sleep to avoid interfering with MP3 SoftwareSerial
       * Optional passthrough available when BT_PASSTHROUGH == 1
   - MP3:
       * MP3 control runs ONLY when MP3 source selected
   - Display:
       * temporary behavior:
         - if folder == 3 -> pulse PWM
         - else          -> solid brightness
*/

#include <Arduino.h>
#include "Config.h"
#include "Radio_Tuning.h"
#include "Display_LED.h"
#include "Bluetooth.h"
#include "MP3.h"

// -------------------- Compile-time debugging controls --------------------
// Set to 1 if you want Serial <-> BT passthrough for AT debugging.
// Leave at 0 for normal operation.
#define BT_PASSTHROUGH 0

// Set to 1 once the tuner RC input is connected and calibrated.
#define TUNER_CONNECTED 0

// When tuner is not connected, force a default folder so MP3 can still play.
// Folder scheme currently: 0..3 (later we’ll formalise this in MP3 API step).
constexpr uint8_t DEFAULT_FOLDER_WHEN_NO_TUNER = 0;

// Shared state
static uint8_t g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;
static uint8_t s_displayBrightness = 60;

// Adaptive tuning schedule
static unsigned long g_lastTuneMs = 0;
constexpr uint16_t   TUNE_FAST_MS = 300; // when folder == 3
constexpr uint16_t   TUNE_SLOW_MS = 600; // when folder != 3

// Bluetooth module (pins from Config.h)
static BluetoothModule btModule(Config::PIN_BT_RX, Config::PIN_BT_TX);

// Track mode changes for clean debug output
enum class SourceMode : uint8_t { MP3, Bluetooth };
static SourceMode g_lastMode = SourceMode::Bluetooth;

static SourceMode readSourceMode() {
  // LOW = MP3 selected, HIGH = Bluetooth selected (pull-up) [4](https://github.com/jcobris/Vintage-Radio-1-public/compare)
  const bool mp3Selected = (digitalRead(Config::PIN_SOURCE_DETECT) == LOW);
  return mp3Selected ? SourceMode::MP3 : SourceMode::Bluetooth;
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
  // Sleep BT SoftwareSerial so it doesn't interfere with MP3 SoftwareSerial
  btModule.sleep();
  Serial.println(F("[BOOT] BT201 UART slept (passthrough disabled)."));
#else
  Serial.println(F("[BOOT] BT201 UART active (passthrough enabled)."));
#endif

  // ---- MP3 init ----
  MP3::init();
  Serial.println(F("[BOOT] MP3 init done."));

  // Capture initial mode
  g_lastMode = readSourceMode();
  Serial.print(F("[MODE] Initial source: "));
  Serial.println((g_lastMode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));
}

void loop() {
  const unsigned long now = millis();

  const SourceMode mode = readSourceMode();
  if (mode != g_lastMode) {
    g_lastMode = mode;
    Serial.print(F("[MODE] Source changed to: "));
    Serial.println((mode == SourceMode::MP3) ? F("MP3") : F("Bluetooth"));
  }

  if (mode == SourceMode::MP3) {
    // 1) Folder selection
#if TUNER_CONNECTED == 1
    const uint16_t tuneInterval = (g_currentFolder == 3) ? TUNE_FAST_MS : TUNE_SLOW_MS;
    if (now - g_lastTuneMs >= tuneInterval) {
      g_currentFolder = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);
      g_lastTuneMs = now;
      // Optional: only print when it changes (keeps output predictable)
      static uint8_t lastPrintedFolder = 255;
      if (g_currentFolder != lastPrintedFolder) {
        lastPrintedFolder = g_currentFolder;
        Serial.print(F("[TUNE] Folder = "));
        Serial.println(g_currentFolder);
      }
    }
#else
    g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;
#endif

    // 2) Display behavior (temporary)
    if (g_currentFolder == 3) {
      DisplayLED::pulseTick(20);
    } else {
      analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);
    }

    // 3) MP3 behavior only when MP3 is selected
    MP3::tick();

  } else {
    // Bluetooth selected:
    // For now: keep display solid
    analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);

#if BT_PASSTHROUGH == 1
    // Only run passthrough when explicitly enabled
    btModule.passthrough();
#endif
  }
}
