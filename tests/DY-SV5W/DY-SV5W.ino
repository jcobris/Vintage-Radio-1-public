
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "MP3Player.h"

// SoftwareSerial pins for MP3 module
#define MP3_RX 11  // Arduino RX (connect to MP3 TX)
#define MP3_TX 12  // Arduino TX (connect to MP3 RX)

SoftwareSerial mp3Serial(MP3_RX, MP3_TX);

// Global folder variable (simulate tuner input)
int g_currentFolder = 1; // 1-4 valid, 99 = stop

unsigned long lastCheck = 0;

void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600); // DY-SV5W default baud rate
  Serial.println("MP3 Player Test Started");
}

void loop() {
  // Echo any data from MP3 module to Serial Monitor
  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
    Serial.print("MP3 -> ");
    Serial.println(incoming, HEX);
  }

  // Check folder variable every 1 second
  if (millis() - lastCheck >= 1000) {
    lastCheck = millis();

    if (g_currentFolder == 99) {
      Serial.println("Stopping playback...");
      stopPlayback(mp3Serial);
    } else if (g_currentFolder >= 1 && g_currentFolder <= 4) {
      Serial.print("Playing random track from folder ");
      Serial.println(g_currentFolder);
      playRandomInFolder(mp3Serial, g_currentFolder);
    }
  }
}
