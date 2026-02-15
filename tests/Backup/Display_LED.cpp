Radio_Main.ino original


/* Radio_Main.ino
   - Uses global debug macros from Config.h (accessible in all modules)
   - Orchestrates:
     * RadioTuning::getFolder(D6) -> returns folder number (0..3)
     * DisplayLED::update(folder) -> pulsate if folder==0, else solid (PWM on D11)
*/

#include <Arduino.h>
#include "Config.h"
#include "Radio_Tuning.h"
#include "Display_LED.h"

// Shared state
static uint8_t g_currentFolder = 0;

void setup() {
  // Serial at 9600 (your preference); only needed if DEBUG == 1
  #if DEBUG == 1
    Serial.begin(9600);
    while (!Serial) { /* for native USB boards; harmless elsewhere */ }
  #endif

  // Pin setup
  pinMode(Config::PIN_LED_DISPLAY, OUTPUT);
  // RC timing routine will manage D6 modes internally (OUTPUT to discharge, INPUT to measure)

  // Initialize LED module
  DisplayLED::init(Config::PIN_LED_DISPLAY);
  
  // TEMP: sanity check PWM control on D11
  analogWrite(Config::PIN_LED_DISPLAY, 0);     // should turn LEDs fully off
  delay(1500);
  analogWrite(Config::PIN_LED_DISPLAY, 255);   // should turn LEDs full on
  delay(1500);


  debugln("Setup complete.");
}

void loop() {
  // Measure RC timing on D6 and map to folder 0..3
  g_currentFolder = RadioTuning::getFolder(Config::PIN_TUNING_INPUT);

  // Drive LED according to folder selection
  DisplayLED::update(g_currentFolder);

  // Optional: print only on change to keep logs light
  static uint8_t lastPrinted = 255;
  if (DEBUG && (lastPrinted != g_currentFolder)) {
    debug("Folder: ");
    debugln(g_currentFolder);
    lastPrinted = g_currentFolder;
  }
}
