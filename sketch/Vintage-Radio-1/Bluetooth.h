
// Bluetooth.h
#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>
#include <SoftwareSerial.h>

class BluetoothModule {
public:
  // rxPin = Arduino pin connected to BT201 TX
  // txPin = Arduino pin connected to BT201 RX
  BluetoothModule(uint8_t rxPin, uint8_t txPin);

  // Start SoftwareSerial at the requested baud
  void begin(long baudRate);

  // Send initial AT configuration commands
  void sendInitialCommands();

  // Optional debug passthrough (PC Serial <-> BT module)
  // Only works if the Bluetooth UART is active (not slept).
  void passthrough();

  // Disable Bluetooth SoftwareSerial to avoid interfering with MP3 SoftwareSerial
  void sleep();

  // Re-enable Bluetooth SoftwareSerial (uses last baud set by begin())
  void wake();

  bool isActive() const;

private:
  uint8_t _rxPin;
  uint8_t _txPin;
  long    _baud;
  bool    _active;

  SoftwareSerial btSerial;

  void sendCommand(const String &cmd);
};

#endif // BLUETOOTH_H
