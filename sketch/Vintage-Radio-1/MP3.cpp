
#include "MP3.h"
#include "Config.h"
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

/*
============================================================
DY-SV5W UART Protocol Notes
============================================================
Commands are sent as byte frames beginning with 0xAA.
Many commands are fixed frames; volume/EQ commands use a checksum.
Timing / stability:
 - Work occurs mainly on desired-folder changes at a 500ms cadence.

SRAM optimization:
 - Fixed command frames are stored in PROGMEM to avoid consuming .data SRAM.
*/

static SoftwareSerial mp3Serial(Config::PIN_MP3_RX, Config::PIN_MP3_TX);

#define MP3_ONLINE_TIMEOUT_MS 5000UL

static uint8_t volume = 30;

// Desired folder set by main sketch (1..4, or 99=mute)
static volatile uint8_t s_desiredFolder = 1;

// Internal tracking of current module folder (1..4)
static int mp3Folder = 1;

// Tick timing and one-shot gating
static unsigned long lastCheck = 0;
static bool folderSelected = false;
static uint8_t lastDesiredSeen = 255;

static bool s_mp3Online = false;
static bool s_isMuted = false;

// ------------------------------------------------------------
// Fixed command frames (PROGMEM to save SRAM)
// ------------------------------------------------------------
static const uint8_t PROGMEM CMD_CHECK_ONLINE[]           = {0xAA, 0x09, 0x00, 0xB3};
static const uint8_t PROGMEM CMD_VOL_MUTE[]               = {0xAA, 0x13, 0x01, 0x00, 0xBE}; // volume=0
static const uint8_t PROGMEM CMD_SET_SD[]                 = {0xAA, 0x0B, 0x01, 0x01, 0xB7};
static const uint8_t PROGMEM CMD_NEXT_FOLDER[]            = {0xAA, 0x0F, 0x00, 0xB9};
static const uint8_t PROGMEM CMD_PREV_FOLDER[]            = {0xAA, 0x0E, 0x00, 0xB8};
static const uint8_t PROGMEM CMD_PLAY_RANDOM_IN_FOLDER[]  = {0xAA, 0x18, 0x01, 0x05, 0xC8};
static const uint8_t PROGMEM CMD_PLAY[]                   = {0xAA, 0x02, 0x00, 0xAC};
static const uint8_t PROGMEM CMD_NEXT_TRACK[]             = {0xAA, 0x06, 0x00, 0xB0};

// ------------------------------------------------------------

static uint8_t calcChecksum(const uint8_t *buf, uint8_t len_without_checksum) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len_without_checksum; ++i) sum += buf[i];
  return (uint8_t)(sum & 0xFF);
}

static void logTxFrame_P(const uint8_t *cmdP, uint8_t len) {
  if (!(DEBUG == 1 && MP3_FRAME_DEBUG == 1)) {
    (void)cmdP; (void)len;
    return;
  }
  debug(F("MP3 TX: "));
  for (uint8_t i = 0; i < len; i++) {
    const uint8_t b = pgm_read_byte(&cmdP[i]);
    if (b < 16) debug('0');
    Serial.print(b, HEX);
    Serial.print(' ');
  }
  Serial.println();
}

static void logTxFrame_RAM(const uint8_t *cmd, uint8_t len) {
  if (!(DEBUG == 1 && MP3_FRAME_DEBUG == 1)) {
    (void)cmd; (void)len;
    return;
  }
  debug(F("MP3 TX: "));
  for (uint8_t i = 0; i < len; i++) {
    const uint8_t b = cmd[i];
    if (b < 16) debug('0');
    Serial.print(b, HEX);
    Serial.print(' ');
  }
  Serial.println();
}

static void sendCommand_P(const uint8_t *cmdP, uint8_t len) {
  logTxFrame_P(cmdP, len);
  mp3Serial.listen();
  for (uint8_t i = 0; i < len; ++i) {
    mp3Serial.write(pgm_read_byte(&cmdP[i]));
  }
  delay(20);
}

static void sendCommand_RAM(const uint8_t *cmd, uint8_t len) {
  logTxFrame_RAM(cmd, len);
  mp3Serial.listen();
  mp3Serial.write(cmd, len);
  delay(20);
}

static void sendSetVolume(uint8_t vol) {
  if (vol > 30) vol = 30;
  uint8_t frame[5] = { 0xAA, 0x13, 0x01, vol, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand_RAM(frame, 5);
}

static void sendSetEQ(uint8_t mode) {
  if (mode > 4) mode = 0;
  uint8_t frame[5] = { 0xAA, 0x1A, 0x01, mode, 0x00 };
  frame[4] = calcChecksum(frame, 4);
  sendCommand_RAM(frame, 5);
}

static void playRandomTrack() {
  sendCommand_P(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));

  // Restore volume after mute, before play
  if (s_isMuted) {
    sendSetVolume(volume);
    s_isMuted = false;
    DBG_MP3_KV(F("MP3: Volume restored to "), volume);
  }

  sendCommand_P(CMD_PLAY, sizeof(CMD_PLAY));
  DBG_MP3(F("MP3: Playback started (random-in-folder)."));
}

static void syncFolderTo(int targetFolder /*1..4*/) {
  while (mp3Folder != targetFolder) {
    if (targetFolder > mp3Folder) {
      sendCommand_P(CMD_NEXT_FOLDER, sizeof(CMD_NEXT_FOLDER));
      mp3Folder++;
    } else {
      sendCommand_P(CMD_PREV_FOLDER, sizeof(CMD_PREV_FOLDER));
      mp3Folder--;
    }
  }
  DBG_MP3_KV(F("MP3: Folder synced to "), mp3Folder);
}

static void initialSetup() {
  sendCommand_P(CMD_SET_SD, sizeof(CMD_SET_SD));
  delay(50);
  sendSetVolume(volume);
  delay(50);
  sendSetEQ(1);
  delay(50);
  sendCommand_P(CMD_PLAY_RANDOM_IN_FOLDER, sizeof(CMD_PLAY_RANDOM_IN_FOLDER));
  delay(50);
  sendCommand_P(CMD_PLAY, sizeof(CMD_PLAY));
  delay(100);
  s_isMuted = false;
}

static bool checkMP3OnlineWithTimeout(unsigned long timeoutMs) {
  const unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    sendCommand_P(CMD_CHECK_ONLINE, sizeof(CMD_CHECK_ONLINE));
    delay(250);
    if (mp3Serial.available()) {
      while (mp3Serial.available()) (void)mp3Serial.read();
      DBG_MP3(F("MP3: Online"));
      return true;
    }
  }
  DBG_MP3(F("MP3: Offline (timeout)"));
  return false;
}

// -------------------- Public API --------------------
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
  DBG_MP3(F("MP3: Control Ready"));

  s_mp3Online = checkMP3OnlineWithTimeout(MP3_ONLINE_TIMEOUT_MS);
  if (!s_mp3Online) {
    DBG_MP3(F("[WARN] MP3 not responding; continuing without MP3."));
    return;
  }
  initialSetup();
}

void MP3::nextTrack() {
  if (!s_mp3Online) return;
  sendCommand_P(CMD_NEXT_TRACK, sizeof(CMD_NEXT_TRACK));
  DBG_MP3(F("MP3: Next track"));
}

void MP3::tick() {
  if (!s_mp3Online) return;

  // Drain RX (optional debug)
  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
    if (DEBUG == 1 && MP3_RX_DEBUG == 1) {
      debug(F("MP3 RX: "));
      if (incoming < 16) debug('0');
      Serial.println(incoming, HEX);
    } else {
      (void)incoming;
    }
  }

  // Act only every 500ms to reduce command traffic
  if (millis() - lastCheck >= 500) {
    lastCheck = millis();

    const uint8_t desired = s_desiredFolder;

    // Detect folder change
    if (desired != lastDesiredSeen) {
      lastDesiredSeen = desired;
      folderSelected = false;
      DBG_MP3_KV(F("MP3: Desired folder = "), desired);
    }

    // One-shot action per desired folder value
    if (!folderSelected) {
      if (desired == 99) {
        DBG_MP3(F("MP3: Mute requested (99)"));
        sendCommand_P(CMD_VOL_MUTE, sizeof(CMD_VOL_MUTE));
        s_isMuted = true;
      } else {
        syncFolderTo((int)desired);
        playRandomTrack();
      }
      folderSelected = true;
    }
  }
}
