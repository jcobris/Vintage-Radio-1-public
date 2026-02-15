
#include <Arduino.h>
#include <SoftwareSerial.h>

// -------------------- Wiring --------------------
// DY-SV5W TXD -> Arduino MP3_RX (software serial RX)
// DY-SV5W RXD -> Arduino MP3_TX (software serial TX)  [Add level protection if 5V MCU]
// GNDs common. Module UART is 3.3V logic.
// ------------------------------------------------

#define MP3_RX 11   // DY-SV5W TX -> Arduino RX pin
#define MP3_TX 12   // DY-SV5W RX -> Arduino TX pin
SoftwareSerial mp3Serial(MP3_RX, MP3_TX);

// --------- USER TWEAKABLE ---------
uint8_t volume = 30;   // 0..30 (DY-SV5W volume scale). Change this anytime.
// uint8_t volume = 30; // Example: uncomment for max volume
// ----------------------------------

// Debug / state
int mp3Debug = 1;
int g_currentFolder = 1;
int mp3Folder = 1;
unsigned long lastCheck = 0;
bool folderSelected = false;

// --- Prebuilt commands that don't change ---
// NOTE: Frames are: AA <CMD> <LEN> [DATA...] <CHK> where CHK = low 8-bits of the sum.
static const byte CMD_CHECK_ONLINE[]          = {0xAA, 0x09, 0x00, 0xB3};
static const byte CMD_VOL_MUTE[]              = {0xAA, 0x13, 0x01, 0x00, 0xBE}; // volume=0
static const byte CMD_SET_SD[]                = {0xAA, 0x0B, 0x01, 0x01, 0xB7}; // select SD device
static const byte CMD_NEXT_FOLDER[]           = {0xAA, 0x0F, 0x00, 0xB9};
static const byte CMD_PREV_FOLDER[]           = {0xAA, 0x0E, 0x00, 0xB8};

// EQ = POP (00=Normal, 01=Pop, 02=Rock, 03=Jazz, 04=Classic)
static const byte CMD_EQ_POP[]                = {0xAA, 0x1A, 0x01, 0x01, 0xC6};

// Play mode = Random in Folder
static const byte CMD_PLAY_RANDOM_IN_FOLDER[] = {0xAA, 0x18, 0x01, 0x05, 0xC8};

// Play
static const byte CMD_PLAY[]                  = {0xAA, 0x02, 0x00, 0xAC};

// ----------------- Helpers -----------------

// Pretty-printer + write
void sendCommand(const byte *cmd, int len) {
  if (mp3Debug) {
    Serial.print(F("Sending: "));
    for (int i = 0; i < len; i++) {
      if (cmd[i] < 16) Serial.print('0');
      Serial.print(cmd[i], HEX);
      Serial.print(' ');
    }
    Serial.println();
  }
  mp3Serial.write(cmd, len);
  delay(20); // DY-SV5W is slow to accept back-to-back frames (9600 8N1)
}

// Checksum = low 8 bits of sum of all bytes before checksum
uint8_t calcChecksum(const uint8_t *buf, uint8_t len_without_checksum) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len_without_checksum; ++i) sum += buf[i];
  return (uint8_t)(sum & 0xFF);
}

// Build & send "Set Volume" frame: AA 13 01 <VOL> <CHK> (VOL 0..30)
void sendSetVolume(uint8_t vol /*0..30*/) {
  if (vol > 30) vol = 30;
  uint8_t frame[5] = { 0xAA, 0x13, 0x01, vol, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand(frame, 5);
}

// OPTIONAL: Build & send "Set EQ" frame: AA 1A 01 <MODE> <CHK>
// MODE: 00=Normal, 01=Pop, 02=Rock, 03=Jazz, 04=Classic
void sendSetEQ(uint8_t mode /*0..4*/) {
  if (mode > 4) mode = 0;
  uint8_t frame[5] = { 0xAA, 0x1A, 0x01, mode, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand(frame, 5);
}

// Wrapper to set random-in-folder + play
void playRandomTrack() {
  sendCommand(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));
  sendCommand(CMD_PLAY, sizeof(CMD_PLAY));
  if (mp3Debug) Serial.println(F("Random-in-folder mode set, playback started."));
}

// Keep advancing/retreating until mp3Folder == g_currentFolder
void syncFolder() {
  while (mp3Folder != g_currentFolder) {
    if (g_currentFolder > mp3Folder) {
      sendCommand(CMD_NEXT_FOLDER, sizeof(CMD_NEXT_FOLDER));
      mp3Folder++;
    } else {
      sendCommand(CMD_PREV_FOLDER, sizeof(CMD_PREV_FOLDER));
      mp3Folder--;
    }
  }
  if (mp3Debug) {
    Serial.print(F("Folder synced: "));
    Serial.println(mp3Folder);
  }
}

// ----------------- Initialization -----------------

// Init sequence requested:
// 1) Select SD
// 2) Set volume (from global 'volume')
// 3) EQ = POP
// 4) Play mode: Random in Folder
// 5) PLAY
void initialSetup() {
  sendCommand(CMD_SET_SD, sizeof(CMD_SET_SD));
  delay(50);

  sendSetVolume(volume);  // uses variable at top (0..30)
  delay(50);

  // You can use the fixed POP command or the helper:
  // sendCommand(CMD_EQ_POP, sizeof(CMD_EQ_POP));
  sendSetEQ(1); // 1 = POP
  delay(50);

  sendCommand(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));
  delay(50);

  sendCommand(CMD_PLAY, sizeof(CMD_PLAY));
  delay(100);
}

void checkMP3Online() {
  while (true) {
    sendCommand(CMD_CHECK_ONLINE, sizeof(CMD_CHECK_ONLINE));
    delay(500);
    if (mp3Serial.available()) {
      if (mp3Debug) Serial.println(F("MP3 Online"));
      // Flush any returned bytes
      while (mp3Serial.available()) (void)mp3Serial.read();
      break;
    }
    if (mp3Debug) Serial.println(F("MP3 Offline, retrying..."));
  }
}

// ----------------- Arduino lifecycle -----------------

void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600); // DY-SV5W fixed UART parameters: 9600 8N1
  if (mp3Debug) Serial.println(F("MP3 Control Ready"));

  checkMP3Online();
  initialSetup();
}

void loop() {
  // Drain any module bytes (module is mostly fire-and-forget, but we read just in case)
  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
    if (mp3Debug) {
      Serial.print(F("MP3 -> "));
      if (incoming < 16) Serial.print('0');
      Serial.println(incoming, HEX);
    }
  }

  if (millis() - lastCheck >= 500) {
    lastCheck = millis();

    if (g_currentFolder != mp3Folder) folderSelected = false;

    if (!folderSelected) {
      if (g_currentFolder == 99) {
        if (mp3Debug) Serial.println(F("Tuner folder = 99, muting..."));
        sendCommand(CMD_VOL_MUTE, sizeof(CMD_VOL_MUTE));
      } else {
        syncFolder();
        playRandomTrack();
      }
      folderSelected = true;
    }
  }
}

