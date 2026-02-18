
#include "MP3.h"
#include "Config.h"
#include <Arduino.h>
#include <SoftwareSerial.h>

static SoftwareSerial mp3Serial(Config::PIN_MP3_RX, Config::PIN_MP3_TX);

// Debug controls
#define MP3_DEBUG_EVENTS 1
#define MP3_DEBUG_FRAMES 0
#define MP3_DEBUG_RX     0
#define MP3_SERIAL_CONTROL 0

#define MP3_ONLINE_TIMEOUT_MS 5000UL

static uint8_t volume = 30;

// Desired folder set by main sketch (1..4, or 99=mute)
static volatile uint8_t s_desiredFolder = 1;

// Internal tracking of current module folder (1..4)
static int mp3Folder = 1;

static unsigned long lastCheck = 0;
static bool folderSelected = false;
static uint8_t lastDesiredSeen = 255;

static bool s_mp3Online = false;

// Commands
static const byte CMD_CHECK_ONLINE[] = {0xAA, 0x09, 0x00, 0xB3};
static const byte CMD_VOL_MUTE[]     = {0xAA, 0x13, 0x01, 0x00, 0xBE};
static const byte CMD_SET_SD[]       = {0xAA, 0x0B, 0x01, 0x01, 0xB7};
static const byte CMD_NEXT_FOLDER[]  = {0xAA, 0x0F, 0x00, 0xB9};
static const byte CMD_PREV_FOLDER[]  = {0xAA, 0x0E, 0x00, 0xB8};
static const byte CMD_PLAY_RANDOM_IN_FOLDER[] = {0xAA, 0x18, 0x01, 0x05, 0xC8};
static const byte CMD_PLAY[] = {0xAA, 0x02, 0x00, 0xAC};

static void logTxFrame(const byte *cmd, int len) {
#if MP3_DEBUG_FRAMES == 1
  Serial.print(F("MP3 TX: "));
  for (int i = 0; i < len; i++) {
    if (cmd[i] < 16) Serial.print('0');
    Serial.print(cmd[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
#else
  (void)cmd; (void)len;
#endif
}

static void sendCommand(const byte *cmd, int len) {
  logTxFrame(cmd, len);
  mp3Serial.listen();
  mp3Serial.write(cmd, len);
  delay(20);
}

static uint8_t calcChecksum(const uint8_t *buf, uint8_t len_without_checksum) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len_without_checksum; ++i) sum += buf[i];
  return (uint8_t)(sum & 0xFF);
}

static void sendSetVolume(uint8_t vol) {
  if (vol > 30) vol = 30;
  uint8_t frame[5] = { 0xAA, 0x13, 0x01, vol, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand(frame, 5);
}

static void sendSetEQ(uint8_t mode) {
  if (mode > 4) mode = 0;
  uint8_t frame[5] = { 0xAA, 0x1A, 0x01, mode, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand(frame, 5);
}

static void playRandomTrack() {
  sendCommand(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));
  sendCommand(CMD_PLAY, sizeof(CMD_PLAY));
#if MP3_DEBUG_EVENTS == 1
  Serial.println(F("MP3: Playback started (random-in-folder)."));
#endif
}

static void syncFolderTo(int targetFolder /*1..4*/) {
  while (mp3Folder != targetFolder) {
    if (targetFolder > mp3Folder) {
      sendCommand(CMD_NEXT_FOLDER, sizeof(CMD_NEXT_FOLDER));
      mp3Folder++;
    } else {
      sendCommand(CMD_PREV_FOLDER, sizeof(CMD_PREV_FOLDER));
      mp3Folder--;
    }
  }
#if MP3_DEBUG_EVENTS == 1
  Serial.print(F("MP3: Folder synced to "));
  Serial.println(mp3Folder);
#endif
}

static void initialSetup() {
  sendCommand(CMD_SET_SD, sizeof(CMD_SET_SD));
  delay(50);

  sendSetVolume(volume);
  delay(50);

  sendSetEQ(1);
  delay(50);

  sendCommand(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));
  delay(50);
  sendCommand(CMD_PLAY, sizeof(CMD_PLAY));
  delay(100);
}

static bool checkMP3OnlineWithTimeout(unsigned long timeoutMs) {
  const unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    sendCommand(CMD_CHECK_ONLINE, sizeof(CMD_CHECK_ONLINE));
    delay(250);

    if (mp3Serial.available()) {
      while (mp3Serial.available()) (void)mp3Serial.read();
#if MP3_DEBUG_EVENTS == 1
      Serial.println(F("MP3: Online"));
#endif
      return true;
    }
  }

#if MP3_DEBUG_EVENTS == 1
  Serial.println(F("MP3: Offline (timeout)"));
#endif
  return false;
}

// Public API
void MP3::setDesiredFolder(uint8_t folder) {
  // Accept 1..4 or 99. Anything else clamps to 1.
  if ((folder >= 1 && folder <= 4) || (folder == 99)) {
    s_desiredFolder = folder;
  } else {
    s_desiredFolder = 1;
  }
}

uint8_t MP3::getDesiredFolder() {
  return s_desiredFolder;
}

void MP3::init() {
  mp3Serial.begin(Config::MP3_BAUD);
  mp3Serial.listen();

#if MP3_DEBUG_EVENTS == 1
  Serial.println(F("MP3: Control Ready"));
#endif

  s_mp3Online = checkMP3OnlineWithTimeout(MP3_ONLINE_TIMEOUT_MS);
  if (!s_mp3Online) {
#if MP3_DEBUG_EVENTS == 1
    Serial.println(F("[WARN] MP3 not responding; continuing without MP3."));
#endif
    return;
  }

  initialSetup();
}

void MP3::tick() {
  if (!s_mp3Online) return;

  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
#if (MP3_DEBUG_FRAMES == 1) || (MP3_DEBUG_RX == 1)
    Serial.print(F("MP3 RX: "));
    if (incoming < 16) Serial.print('0');
    Serial.println(incoming, HEX);
#else
    (void)incoming;
#endif
  }

  if (millis() - lastCheck >= 500) {
    lastCheck = millis();

    const uint8_t desired = s_desiredFolder;

    if (desired != lastDesiredSeen) {
      lastDesiredSeen = desired;
      folderSelected = false;
#if MP3_DEBUG_EVENTS == 1
      Serial.print(F("MP3: Desired folder = "));
      Serial.println(desired);
#endif
    }

    if (!folderSelected) {
      if (desired == 99) {
#if MP3_DEBUG_EVENTS == 1
        Serial.println(F("MP3: Mute requested (99)"));
#endif
        sendCommand(CMD_VOL_MUTE, sizeof(CMD_VOL_MUTE));
      } else {
        // desired is already 1..4 now
        syncFolderTo((int)desired);
        playRandomTrack();
      }
      folderSelected = true;
    }
  }
}
