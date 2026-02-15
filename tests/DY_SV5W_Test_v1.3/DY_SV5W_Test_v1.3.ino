
#include <Arduino.h>
#include <SoftwareSerial.h>

#define MP3_RX 11
#define MP3_TX 12

SoftwareSerial mp3Serial(MP3_RX, MP3_TX);

int mp3Debug = 1;             // 0 = no debug, 1 = debug on
int g_currentFolder = 1;      // Simulated tuner folder
int mp3Folder = 1;            // Assume MP3 starts at folder 1
unsigned long lastCheck = 0;
bool folderSelected = false;  // Tracks if current folder has been handled

void sendCommand(const byte *cmd, int len) {
  if (mp3Debug == 1) {
    Serial.print("Sending: ");
    for (int i = 0; i < len; i++) {
      Serial.print(cmd[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  for (int i = 0; i < len; i++) {
    mp3Serial.write(cmd[i]);
  }
  delay(200); // Allow MP3 module to process
}

// Calculate checksum for dynamic commands
byte calcChecksum(const byte *cmd, int len) {
  int sum = 0;
  for (int i = 0; i < len; i++) sum += cmd[i];
  return (0x100 - (sum % 256));
}

// Query number of tracks in current folder
int getTrackCount(int folder) {
  byte cmd[] = {0xAA, 0x12, 0x02, (folder >> 8) & 0xFF, folder & 0xFF, 0x00};
  cmd[5] = calcChecksum(cmd, 5);
  sendCommand(cmd, sizeof(cmd));

  delay(300); // Wait for response
  byte response[6];
  int idx = 0;
  while (mp3Serial.available() && idx < 6) {
    response[idx++] = mp3Serial.read();
  }

  if (idx >= 6 && response[1] == 0x12) {
    int count = (response[3] << 8) | response[4]; // High + Low byte
    if (mp3Debug == 1) {
      Serial.print("Track count: ");
      Serial.println(count);
    }
    return count > 0 ? count : 1;
  }

  if (mp3Debug == 1) Serial.println("Failed to read track count, defaulting to 1");
  return 1;
}

// Play a specific track by index
void playTrackByIndex(int trackNum) {
  byte cmd[] = {0xAA, 0x1F, 0x02, (trackNum >> 8) & 0xFF, trackNum & 0xFF, 0x00};
  cmd[5] = calcChecksum(cmd, 5);
  sendCommand(cmd, sizeof(cmd));

  const byte vol20[] = {0xAA, 0x13, 0x01, 0x14, 0xD2}; // Restore volume
  sendCommand(vol20, sizeof(vol20));

  if (mp3Debug == 1) {
    Serial.print("Playing track: ");
    Serial.println(trackNum);
  }
}

void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600);
  if (mp3Debug == 1) Serial.println("MP3 Control Ready");

  const byte checkOnline[] = {0xAA, 0x09, 0x00, 0xB3};
  while (true) {
    sendCommand(checkOnline, sizeof(checkOnline));
    delay(500);
    if (mp3Serial.available()) {
      if (mp3Debug == 1) Serial.println("MP3 Online");
      break;
    }
    if (mp3Debug == 1) Serial.println("MP3 Offline, retrying...");
  }

  const byte volMute[] = {0xAA, 0x13, 0x01, 0x00, 0xBE};
  const byte setSD[]   = {0xAA, 0x0B, 0x01, 0x01, 0xB7};
  const byte eqRock[]  = {0xAA, 0x07, 0x01, 0x02, 0xB4};

  sendCommand(volMute, sizeof(volMute));
  sendCommand(setSD, sizeof(setSD));
  sendCommand(eqRock, sizeof(eqRock));
}

void loop() {
  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
    if (mp3Debug == 1) {
      Serial.print("MP3 -> ");
      Serial.println(incoming, HEX);
    }
  }

  if (millis() - lastCheck >= 500) {
    lastCheck = millis();

    if (g_currentFolder != mp3Folder) {
      folderSelected = false;
    }

    if (!folderSelected) {
      const byte nextFolder[] = {0xAA, 0x0F, 0x00, 0xB9};
      const byte prevFolder[] = {0xAA, 0x0E, 0x00, 0xB8};
      const byte volMute[]    = {0xAA, 0x13, 0x01, 0x00, 0xBE};

      if (g_currentFolder == 99) {
        if (mp3Debug == 1) Serial.println("Tuner folder = 99, muting...");
        sendCommand(volMute, sizeof(volMute));
        folderSelected = true;
      } else {
        while (mp3Folder != g_currentFolder) {
          if (g_currentFolder > mp3Folder) {
            sendCommand(nextFolder, sizeof(nextFolder));
            mp3Folder++;
          } else {
            sendCommand(prevFolder, sizeof(prevFolder));
            mp3Folder--;
          }
        }
        if (mp3Debug == 1) {
          Serial.print("Folder synced: ");
          Serial.println(mp3Folder);
        }

        // Query track count and play random track
        int trackCount = getTrackCount(mp3Folder);
        int randomTrack = random(1, trackCount + 1);
        playTrackByIndex(randomTrack);

        folderSelected = true;
      }
    }
  }
}










