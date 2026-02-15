
#include <Arduino.h>
#include <SoftwareSerial.h>

#define MP3_RX 11
#define MP3_TX 12

SoftwareSerial mp3Serial(MP3_RX, MP3_TX);

int g_currentFolder = 4;      // Simulated tuner folder
int mp3Folder = 1;            // Assume MP3 starts at folder 1
unsigned long lastCheck = 0;
bool folderSelected = false;  // Tracks if current folder has been handled

void sendCommand(const byte *cmd, int len) {
  Serial.print("Sending: ");
  for (int i = 0; i < len; i++) {
    Serial.print(cmd[i], HEX);
    Serial.print(" ");
    mp3Serial.write(cmd[i]);
  }
  Serial.println();
  delay(20); // Allow MP3 module to process
}

// Subroutine for random play and volume restore
void playRandomTrack() {
  const byte randomMode[] = {0xAA, 0x18, 0x01, 0x05, 0xC8};
  const byte stopCmd[]    = {0xAA, 0x04, 0x00, 0xAE};
  const byte playCmd[]    = {0xAA, 0x02, 0x00, 0xAC};
  const byte vol20[]      = {0xAA, 0x13, 0x01, 0x14, 0xD2};

  sendCommand(randomMode, sizeof(randomMode));
  sendCommand(stopCmd, sizeof(stopCmd));
  sendCommand(playCmd, sizeof(playCmd));
  sendCommand(vol20, sizeof(vol20));
}

void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600);
  Serial.println("MP3 Control Ready");

  // Check MP3 online
  const byte checkOnline[] = {0xAA, 0x09, 0x00, 0xB3};
  while (true) {
    sendCommand(checkOnline, sizeof(checkOnline));
    delay(500);
    if (mp3Serial.available()) {
      Serial.println("MP3 Online");
      break;
    }
    Serial.println("MP3 Offline, retrying...");
  }

  // Initial setup
  const byte volMute[] = {0xAA, 0x13, 0x01, 0x00, 0xBE};
  const byte setSD[]   = {0xAA, 0x0B, 0x01, 0x01, 0xB7};
  const byte eqRock[]  = {0xAA, 0x07, 0x01, 0x02, 0xB4};
  const byte selectTrack1[] = {0xAA, 0x1F, 0x02, 0x00, 0x01, 0xCC};

  sendCommand(volMute, sizeof(volMute));
  sendCommand(setSD, sizeof(setSD));
  sendCommand(eqRock, sizeof(eqRock));
  sendCommand(selectTrack1, sizeof(selectTrack1));
}

void loop() {
  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
    Serial.print("MP3 -> ");
    Serial.println(incoming, HEX);
  }

  if (millis() - lastCheck >= 500) {
    lastCheck = millis();

    // If folder changed, reset flag
    if (g_currentFolder != mp3Folder) {
      folderSelected = false;
    }

    if (!folderSelected) {
      const byte nextFolder[] = {0xAA, 0x0F, 0x00, 0xB9};
      const byte prevFolder[] = {0xAA, 0x0E, 0x00, 0xB8};
      const byte volMute[]    = {0xAA, 0x13, 0x01, 0x00, 0xBE};

      if (g_currentFolder == 99) {
        Serial.println("Tuner folder = 99, muting...");
        sendCommand(volMute, sizeof(volMute));
        folderSelected = true;
      } else {
        // Sync folders in one go
        while (mp3Folder != g_currentFolder) {
          if (g_currentFolder > mp3Folder) {
            sendCommand(nextFolder, sizeof(nextFolder));
            mp3Folder++;
          } else {
            sendCommand(prevFolder, sizeof(prevFolder));
            mp3Folder--;
          }
        }
        Serial.print("Folder synced: ");
        Serial.println(mp3Folder);
        playRandomTrack();
        folderSelected = true;
      }
    }
  }
}





