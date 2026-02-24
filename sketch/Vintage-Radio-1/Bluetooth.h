
// Bluetooth.h
#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>
#include <SoftwareSerial.h>

/*
  ============================================================
  Bluetooth Module (BT201) Wrapper
  ============================================================

  The BT201 is configured via AT commands at boot. After initialization,
  its SoftwareSerial UART is usually put to sleep to prevent contention with
  the MP3 module's SoftwareSerial.

  Why sleep?
  - SoftwareSerial uses interrupt timing and can interfere with other timing-sensitive
    operations (including other SoftwareSerial instances and RC timing reads).
  - Sleeping avoids background listening overhead and reduces jitter.

  Optional passthrough:
  - If enabled at compile time in the sketch, allows PC Serial Monitor <-> BT201 UART.
*/

class BluetoothModule {
public:
  BluetoothModule(uint8_t rxPin, uint8_t txPin);

  // Begin SoftwareSerial at baudRate and set as active listener.
  void begin(long baudRate);

  // Send initial AT commands (stored in flash).
  void sendInitialCommands();

  // Forward data between USB Serial and BT serial (only if UART is active).
  void passthrough();

  // Stop SoftwareSerial and tri-state pins (reduces interference).
  void sleep();

  // Re-enable SoftwareSerial after sleep.
  void wake();

  bool isActive() const;

private:
  uint8_t _rxPin;
  uint8_t _txPin;
  long    _baud;
  bool    _active;
  SoftwareSerial btSerial;

  void sendCommand(const __FlashStringHelper *cmd);
};

#endif // BLUETOOTH_H
