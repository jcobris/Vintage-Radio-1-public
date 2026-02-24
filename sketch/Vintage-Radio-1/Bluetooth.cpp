
// Bluetooth.cpp
#include "Bluetooth.h"
#include "Config.h"

BluetoothModule::BluetoothModule(uint8_t rxPin, uint8_t txPin)
  : _rxPin(rxPin),
    _txPin(txPin),
    _baud(0),
    _active(false),
    btSerial(rxPin, txPin) {
}

void BluetoothModule::begin(long baudRate) {
  _baud = baudRate;
  btSerial.begin(_baud);
  _active = true;
  btSerial.listen();
}

void BluetoothModule::wake() {
  if (_active) return;
  if (_baud <= 0) return;
  btSerial.begin(_baud);
  _active = true;
  btSerial.listen();
}

void BluetoothModule::sleep() {
  if (!_active) return;

  // Stop SoftwareSerial to avoid background ISR timing interference
  btSerial.end();
  _active = false;

  // Tri-state pins
  pinMode(_rxPin, INPUT);
  pinMode(_txPin, INPUT);
}

bool BluetoothModule::isActive() const {
  return _active;
}

void BluetoothModule::sendInitialCommands() {
  if (!_active) wake();

  // Commands stored in flash to save SRAM.
  sendCommand(F("AT+CT04"));            // baud rate setting kept per working setup
  delay(50);
  sendCommand(F("AT+BDO'l Timey Radio"));// device name
  delay(50);
  sendCommand(F("AT+CK00"));            // disable auto-switch
  delay(50);
  sendCommand(F("AT+B200"));            // disable call functions
  delay(50);
  sendCommand(F("AT+CA30"));            // volume 100%
  delay(50);
  sendCommand(F("AT+CN00"));            // disable prompts
  delay(50);
  sendCommand(F("AT+CQ01"));            // EQ Rock
  delay(50);
}

void BluetoothModule::sendCommand(const __FlashStringHelper *cmd) {
  if (!_active) return;
  btSerial.listen();
  btSerial.print(cmd);
  btSerial.print("\r\n");
  delay(100);
}

void BluetoothModule::passthrough() {
  if (!_active) return;

  btSerial.listen();

  // PC -> BT201
  if (Serial.available()) {
    char buf[64];
    size_t n = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
    buf[n] = '\0';
    if (n > 0 && buf[n - 1] == '\r') buf[n - 1] = '\0';

    if (buf[0] != '\0') {
      btSerial.print(buf);
      btSerial.print("\r\n");

      // Only print passthrough debug when DEBUG enabled
      if (DEBUG == 1) {
        Serial.print(F("[DEBUG] Sent to BT: "));
        Serial.println(buf);
      }
    }
  }

  // BT201 -> PC
  while (btSerial.available()) {
    char c = btSerial.read();
    Serial.write(c);
  }
}
