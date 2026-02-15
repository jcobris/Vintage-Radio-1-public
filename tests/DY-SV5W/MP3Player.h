
#ifndef MP3PLAYER_H
#define MP3PLAYER_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// Sends a command to the MP3 module
void sendCommand(SoftwareSerial &serial, const byte *cmd, int len);

// Plays a random track from the specified folder (1-99)
void playRandomInFolder(SoftwareSerial &serial, byte folder);

// Stops playback
void stopPlayback(SoftwareSerial &serial);

#endif
