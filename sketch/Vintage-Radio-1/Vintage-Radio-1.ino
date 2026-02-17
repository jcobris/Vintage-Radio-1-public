
/* Vintage-Radio-1.ino (main)
   - Serial at 115200
   - Source select on D2:
       * D2 LOW  -> MP3 selected
       * D2 HIGH -> Bluetooth selected (INPUT_PULLUP)
     (Audio routing is physical, but code uses this to decide behavior/themes.) [1](https://github.com/jcobris/Vintage-Radio-1-public)
   - Tuning:
       * Can be disabled while tuner hardware is not connected
       * When disabled, uses a default folder so MP3 can still play
   - Display:
       * if folder == 3 -> pulse PWM
       * else           -> solid brightness
   - Bluetooth:
       * Initialise BT module on boot (AT setup commands)
       * Passthrough optional (disabled by default)
*/

#include <Arduino.h>
#include "Config.h"
#include "Radio_Tuning.h"
#include "Display_LED.h"
#include "Bluetooth.h"
#include "MP3.h"

// ------------------------------------------------------------
// Source detect (per project behavior):
// D2 LOW  -> MP3 selected
// D2 HIGH -> Bluetooth selected (use INPUT_PULLUP)
// ------------------------------------------------------------ [1](https://github.com/jcobris/Vintage-Radio-1-public)
constexpr uint8_t PIN_SOURCE_DETECT = 2;

// ------------------------------------------------------------
// Bluetooth wiring (matches your test sketch)
// BT201 TX -> Arduino D10 (RX)
// BT201 RX -> Arduino D9  (TX)
// ------------------------------------------------------------
BluetoothModule btModule(10, 9); // RX, TX pins for BT201

// Set to 1 if you want Serial <-> BT passthrough while debugging AT commands.
// Leave at 0 for normal operation.
#define BT_PASSTHROUGH 0

// ------------------------------------------------------------
// Tuner integration control
// Set to 0 while the tuner RC input is not wired (breadboard phase).
// Set to 1 once the tuner input is connected and calibrated.
// ------------------------------------------------------------
#define TUNER_CONNECTED 0

// When tuner is not connected, force a default folder so MP3 can still play.
// NOTE: This is the "logical" folder number used by your tuning system (0..3).
// We'll map to the MP3 module behavior later if needed.
constexpr uint8_t DEFAULT_FOLDER_WHEN_NO_TUNER = 0; // "folder 0" (typically corresponds to SD folder 1 in your scheme)

// Shared state
static uint8_t g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;
static uint8_t s_displayBrightness = 60;  // Default display brightness

// Adaptive tuning schedule
static unsigned long g_lastTuneMs = 0;
constexpr uint16_t   TUNE_FAST_MS = 300;  // when folder == 3
constexpr uint16_t   TUNE_SLOW_MS = 600;  // when folder != 3

void setup() {
  // Serial for debug
  Serial.begin(115200);
  while (!Serial) { /* harmless on some boards */ }
  delay(200);

  // Source detect
  pinMode(PIN_SOURCE_DETECT, INPUT_PULLUP);

  // Display init
  pinMode(Config::PIN_LED_DISPLAY, OUTPUT);
  DisplayLED::init(Config::PIN_LED_DISPLAY);

  // ---- Bluetooth init ----
  btModule.begin(57600);
  Serial.println(F("[DEBUG] Booting and configuring BT201..."));
  btModule.sendInitialCommands();
  Serial.println(F("[DEBUG] BT201 init complete."));

  // ---- MP3 init ----
  // Initialise MP3 subsystem on boot so it's ready when you flip the physical switch.
  MP3::init();
}

void loop() {
  const unsigned long now = millis();

  // D2 LOW = MP3 selected, D2 HIGH = Bluetooth selected [1](https://github.com/jcobris/Vintage-Radio-1-public)
  const bool mp3Selected = (digitalRead(PIN_SOURCE_DETECT) == LOW);

  if (mp3Selected) {
    // 1) Folder selection
#if TUNER_CONNECTED == 1
    const uint16_t tuneInterval = (g_currentFolder == 3) ? TUNE_FAST_MS : TUNE_SLOW_MS;
    if (now - g_lastTuneMs >= tuneInterval) {
      g_currentFolder = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);
      g_lastTuneMs = now;
    }
#else
    // Tuner not wired yet -> keep a stable default folder
    g_currentFolder = DEFAULT_FOLDER_WHEN_NO_TUNER;
#endif

    // 2) Display behavior (temporary — BT/MP3 themes will be refined later)
    if (g_currentFolder == 3) {
      DisplayLED::pulseTick(20);
    } else {
      analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);
    }

    // 3) Run MP3 behavior only when MP3 is selected
    MP3::tick();

  } else {
    // Bluetooth selected:
    // For now: keep display solid (party theme will come later)
    analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);
  }

  // Optional serial passthrough for AT debugging
#if BT_PASSTHROUGH == 1
  btModule.passthrough();
#endif
}
