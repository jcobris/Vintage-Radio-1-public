
#include "MP3.h"
#include "Config.h"
#include <Arduino.h>
#include <SoftwareSerial.h>

// -------------------- Wiring (from Config.h single source of truth) --------------------
// DY-SV5W TXD -> Arduino PIN_MP3_RX (SoftwareSerial RX)
// DY-SV5W RXD -> Arduino PIN_MP3_TX (SoftwareSerial TX) [Add level shifting if 5V MCU]
// GNDs common. Module UART is 3.3V logic.
// ---------------------------------------------------------------------------------------
static SoftwareSerial mp3Serial(Config::PIN_MP3_RX, Config::PIN_MP3_TX);

// --------- USER TWEAKABLE ---------
static uint8_t volume = 30; // 0..30 (DY-SV5W volume scale)
// ----------------------------------

// Debug
static int mp3Debug = 1;

// Desired folder is now owned by main sketch via MP3::setDesiredFolder()
static volatile uint8_t s_desiredFolder = 0; // 0..3 or 99

// Internal state tracking
static int mp3Folder = 1;                  // module's current folder index tracking (1..n in your stepping logic)
static unsigned long lastCheck = 0;
static bool folderSelected = false;

// --- Prebuilt commands that don't change ---
static const byte CMD_CHECK_ONLINE[] = {0xAA, 0x09, 0x00, 0xB3};
static const byte CMD_VOL_MUTE[]     = {0xAA, 0x13, 0x01, 0x00, 0xBE}; // volume=0
static const byte CMD_SET_SD[]       = {0xAA, 0x0B, 0x01, 0x01, 0xB7}; // select SD device
static const byte CMD_NEXT_FOLDER[]  = {0xAA, 0x0F, 0x00, 0xB9};
static const byte CMD_PREV_FOLDER[]  = {0xAA, 0x0E, 0x00, 0xB8};

// Play mode = Random in Folder
static const byte CMD_PLAY_RANDOM_IN_FOLDER[] = {0xAA, 0x18, 0x01, 0x05, 0xC8};

// Play
static const byte CMD_PLAY[] = {0xAA, 0x02, 0x00, 0xAC};

// ----------------- Helpers -----------------
static void sendCommand(const byte *cmd, int len) {
  if (mp3Debug) {
    Serial.print(F("Sending: "));
    for (int i = 0; i < len; i++) {
      if (cmd[i] < 16) Serial.print('0');
      Serial.print(cmd[i], HEX);
      Serial.print(' ');
    }
    Serial.println();
  }

  mp3Serial.listen();                 // ensure MP3 is active listener
  mp3Serial.write(cmd, len);
  delay(20);                          // module is slow to accept back-to-back frames
}

// Checksum = low 8 bits of sum of all bytes before checksum
static uint8_t calcChecksum(const uint8_t *buf, uint8_t len_without_checksum) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len_without_checksum; ++i) sum += buf[i];
  return (uint8_t)(sum & 0xFF);
}

// Build & send "Set Volume" frame: AA 13 01 <VOL> <CHK> (VOL 0..30)
static void sendSetVolume(uint8_t vol /*0..30*/) {
  if (vol > 30) vol = 30;
  uint8_t frame[5] = { 0xAA, 0x13, 0x01, vol, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand(frame, 5);
}

// Build & send "Set EQ" frame: AA 1A 01 <MODE> <CHK>
// MODE: 00=Normal, 01=Pop, 02=Rock, 03=Jazz, 04=Classic
static void sendSetEQ(uint8_t mode /*0..4*/) {
  if (mode > 4) mode = 0;
  uint8_t frame[5] = { 0xAA, 0x1A, 0x01, mode, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand(frame, 5);
}

static void playRandomTrack() {
  sendCommand(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));
  sendCommand(CMD_PLAY, sizeof(CMD_PLAY));
  if (mp3Debug) Serial.println(F("Random-in-folder mode set, playback started."));
}

// Keep advancing/retreating until mp3Folder == desiredLogicalFolder (0..3 mapped via stepping)
// NOTE: Your existing logic tracks folders by counting next/prev commands.
// We preserve that approach: desired folder is treated as an integer target.
static void syncFolder(int desiredFolder) {
  while (mp3Folder != desiredFolder) {
    if (desiredFolder > mp3Folder) {
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

static void initialSetup() {
  sendCommand(CMD_SET_SD, sizeof(CMD_SET_SD));
  delay(50);

  sendSetVolume(volume);
  delay(50);

  sendSetEQ(1); // 1 = POP
  delay(50);

  sendCommand(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));
  delay(50);

  sendCommand(CMD_PLAY, sizeof(CMD_PLAY));
  delay(100);
}

static void checkMP3Online() {
  while (true) {
    sendCommand(CMD_CHECK_ONLINE, sizeof(CMD_CHECK_ONLINE));
    delay(500);

    if (mp3Serial.available()) {
      if (mp3Debug) Serial.println(F("MP3 Online"));
      while (mp3Serial.available()) (void)mp3Serial.read();
      break;
    }

    if (mp3Debug) Serial.println(F("MP3 Offline, retrying..."));
  }
}

// ----------------- Public API -----------------
void MP3::setDesiredFolder(uint8_t folder) {
  // Accept 0..3 or 99. Anything else clamps to 0.
  if ((folder <= 3) || (folder == 99)) {
    s_desiredFolder = folder;
  } else {
    s_desiredFolder = 0;
  }
}

uint8_t MP3::getDesiredFolder() {
  return s_desiredFolder;
}

// ----------------- Arduino lifecycle -----------------
void MP3::init() {
  // DO NOT call Serial.begin() here. Main .ino owns Serial speed.
  mp3Serial.begin(Config::MP3_BAUD);
  mp3Serial.listen();
  if (mp3Debug) Serial.println(F("MP3 Control Ready"));

  checkMP3Online();
  initialSetup();
}

void MP3::tick() {
  // Drain any module bytes (for debug/visibility)
  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
    if (mp3Debug) {
      Serial.print(F("MP3 -> "));
      if (incoming < 16) Serial.print('0');
      Serial.println(incoming, HEX);
    }
  }

  // Periodic folder / play logic
  if (millis() - lastCheck >= 500) {
    lastCheck = millis();

    const uint8_t desired = s_desiredFolder;

    // If desired changed relative to our tracked mp3Folder, force resync
    if (desired != 99 && (int)desired != mp3Folder) folderSelected = false;
    if (desired == 99) folderSelected = false; // allow repeated mute if needed

    if (!folderSelected) {
      if (desired == 99) {
        if (mp3Debug) Serial.println(F("Desired folder = 99, muting..."));
        sendCommand(CMD_VOL_MUTE, sizeof(CMD_VOL_MUTE));
      } else {
        syncFolder((int)desired);
        playRandomTrack();
      }
      folderSelected = true;
    }
  }
}
