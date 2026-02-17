
/* Vintage-Radio-1.ino (main)
   - Serial at 115200
   - Adaptive tuning scheduler:
       * folder == 3 → sample every ~300 ms
       * folder != 3 → sample every ~600 ms
   - Display:
       * if folder == 3 → pulse PWM via DisplayLED::pulseTick(20)
       * else           → analogWrite brightness (solid)
   - Bluetooth:
       * Initialise BT module on boot (UART setup commands)
       * Passthrough optional (disabled by default)
*/

#include <Arduino.h>
#include "Config.h"
#include "Radio_Tuning.h"
#include "Display_LED.h"
#include "Bluetooth.h"

// ------------------------------------------------------------
// Bluetooth wiring (matches your Bluetooth_Module.ino)
// BT201 TX -> Arduino D10 (RX)
// BT201 RX -> Arduino D9  (TX)
// ------------------------------------------------------------
BluetoothModule btModule(10, 9); // RX, TX pins for BT201

// Set to 1 if you want Serial <-> BT passthrough while debugging AT commands.
// Leave at 0 for normal operation so it doesn't interfere with tuning/display timing.
#define BT_PASSTHROUGH 0

// Shared state
static uint8_t g_currentFolder = 0;
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

  // Display init
  pinMode(Config::PIN_LED_DISPLAY, OUTPUT);
  DisplayLED::init(Config::PIN_LED_DISPLAY);

  // ---- Bluetooth init ----
  // BT201 uses 57600 baud for its UART interface (per your test sketch).
  btModule.begin(57600);

  Serial.println(F("[DEBUG] Booting and configuring BT201..."));

  // Run setup commands every boot (same as your Bluetooth_Module.ino)
  btModule.sendInitialCommands();

  Serial.println(F("[DEBUG] BT201 init complete."));
#if BT_PASSTHROUGH == 1
  Serial.println(F("[DEBUG] BT passthrough enabled. Type AT commands into Serial Monitor."));
#endif
}

void loop() {
  const unsigned long now = millis();

  // 1) Adaptive tuning
  const uint16_t tuneInterval = (g_currentFolder == 3) ? TUNE_FAST_MS : TUNE_SLOW_MS;
  if (now - g_lastTuneMs >= tuneInterval) {
    g_currentFolder = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);
    g_lastTuneMs = now;
  }

  // 2) Display — pulse only for folder 3; otherwise solid brightness
  if (g_currentFolder == 3) {
    DisplayLED::pulseTick(20);  // faster cadence (try 10–20 ms)
  } else {
    analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);
  }

  // 3) Optional: serial passthrough between PC and BT201
#if BT_PASSTHROUGH == 1
  btModule.passthrough();
#endif
}
