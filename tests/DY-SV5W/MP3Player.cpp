
#include "MP3Player.h"

// Basic UART command structure for DY-SV5W:
// Header: 0xAA
// Command byte: varies by function
// Length byte: number of data bytes
// Data bytes: parameters
// Example: Play random in folder -> [0xAA, 0x18, 0x02, folder, 0x00]
// Example: Stop playback -> [0xAA, 0x04, 0x00]

void sendCommand(SoftwareSerial &serial, const byte *cmd, int len) {
  for (int i = 0; i < len; i++) {
    serial.write(cmd[i]);
  }
}

void playRandomInFolder(SoftwareSerial &serial, byte folder) {
  byte cmd[] = {0xAA, 0x18, 0x02, folder, 0x00};
  sendCommand(serial, cmd, sizeof(cmd));
}

void stopPlayback(SoftwareSerial &serial) {
  byte cmd[] = {0xAA, 0x04, 0x00};
  sendCommand(serial, cmd, sizeof(cmd));
}
