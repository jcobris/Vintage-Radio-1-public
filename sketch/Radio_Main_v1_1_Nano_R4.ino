
/* Radio_Main.ino
   - Serial at 115200
   - Adaptive tuning scheduler:
       * folder == 3 → sample every ~300 ms
       * folder != 3 → sample every ~600 ms
   - Display:
       * if folder == 3 → pulse PWM via DisplayLED::pulseTick(10)
       * else           → analog 60 (solid), 
*/

#include <Arduino.h>
#include "Config.h"
#include "Radio_Tuning.h"
#include "Display_LED.h"

// Shared state
static uint8_t g_currentFolder = 0;
static uint8_t s_displayBrightness = 60;  // Default display brightness

// Adaptive tuning schedule
static unsigned long g_lastTuneMs = 0;
constexpr uint16_t   TUNE_FAST_MS = 300;  // when folder == 3
constexpr uint16_t   TUNE_SLOW_MS = 600;  // when folder != 3

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* harmless on UNO R4 Minima */ }

  pinMode(Config::PIN_LED_DISPLAY, OUTPUT);
  DisplayLED::init(Config::PIN_LED_DISPLAY);
}

void loop() {
  const unsigned long now = millis();

  // 1) Adaptive tuning
  const uint16_t tuneInterval = (g_currentFolder == 3) ? TUNE_FAST_MS : TUNE_SLOW_MS;
  if (now - g_lastTuneMs >= tuneInterval) {
    g_currentFolder = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);
    g_lastTuneMs = now;

    // Optional: print only on change
    /*
    static uint8_t lastPrinted = 255;
    if (lastPrinted != g_currentFolder) {
      debug("Folder: "); debugln(g_currentFolder);
      lastPrinted = g_currentFolder;
    }
    */
  }

  // 2) Display — pulse only for folder 3; otherwise digital HIGH
  if (g_currentFolder == 3) {
    DisplayLED::pulseTick(20);              // faster cadence (try 10–20 ms)
  } else {
    analogWrite(Config::PIN_LED_DISPLAY, s_displayBrightness);  // solid ON
  }
}
