
// Bluetooth.cpp
#include "Bluetooth.h"

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
  if (_baud <= 0) return; // begin() must have been called at least once
  btSerial.begin(_baud);
  _active = true;
  btSerial.listen();
}

void BluetoothModule::sleep() {
  if (!_active) return;

  // Stop SoftwareSerial to avoid background listening / interrupts
  btSerial.end();
  _active = false;

  // Tri-state the pins to reduce accidental driving/noise on shared breadboards
  pinMode(_rxPin, INPUT);
  pinMode(_txPin, INPUT);
}

bool BluetoothModule::isActive() const {
  return _active;
}

void BluetoothModule::sendInitialCommands() {
  // Ensure serial is active for init
  if (!_active) wake();

  // All commands stored in flash via F("...") to avoid SRAM / heap churn
  sendCommand(F("AT+CT04"));            // Set baud rate to 57600 (kept per your working setup)
  delay(50);
  sendCommand(F("AT+BDO'l Timey Radio"));// Set Bluetooth name
  delay(50);
  sendCommand(F("AT+CK00"));            // Disable auto-switch to Bluetooth
  delay(50);
  sendCommand(F("AT+B200"));            // Disable Bluetooth call functions
  delay(50);
  sendCommand(F("AT+CA30"));            // Volume 100%
  delay(50);
  sendCommand(F("AT+CN00"));            // Disable prompt sounds
  delay(50);
  sendCommand(F("AT+CQ01"));            // EQ Rock
  delay(50);
}

void BluetoothModule::sendCommand(const __FlashStringHelper *cmd) {
  if (!_active) return;
  btSerial.listen();
  btSerial.print(cmd);
  btSerial.print("\r\n");
  delay(100); // small gap so module can process the command
}

void BluetoothModule::passthrough() {
  if (!_active) return;
  btSerial.listen();

  // PC Serial Monitor -> BT201
  // Avoid String allocation: read a line into a fixed buffer.
  if (Serial.available()) {
    char buf[64];
    size_t n = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
    buf[n] = '\0';

    // Trim trailing \r if present
    if (n > 0 && buf[n - 1] == '\r') {
      buf[n - 1] = '\0';
    }

    if (buf[0] != '\0') {
      btSerial.print(buf);
      btSerial.print("\r\n");
      Serial.print(F("[DEBUG] Sent to BT: "));
      Serial.println(buf);
    }
  }

  // BT201 -> PC Serial Monitor
  while (btSerial.available()) {
    char c = btSerial.read();
    Serial.write(c);
  }
}
