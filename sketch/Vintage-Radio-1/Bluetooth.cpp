
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

  // NOTE: This assumes the BT201 is already configured to use the baud rate
  // you've selected (commonly 57600 in your current setup). [2](https://github.com/jcobris/Vintage-Radio-1-public/security)
  sendCommand("AT+CT04");            // Set baud rate to 57600 (kept per your working setup)
  delay(50);
  sendCommand("AT+BDO'l Timey Radio"); // Set Bluetooth name
  delay(50);
  sendCommand("AT+CK00");            // Disable auto-switch to Bluetooth
  delay(50);
  sendCommand("AT+B200");            // Disable Bluetooth call functions
  delay(50);
  sendCommand("AT+CA30");            // Volume 100%
  delay(50);
  sendCommand("AT+CN00");            // Disable prompt sounds
  delay(50);
  sendCommand("AT+CQ01");            // EQ Rock
  delay(50);
}

void BluetoothModule::sendCommand(const String &cmd) {
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
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      btSerial.print(cmd);
      btSerial.print("\r\n");
      Serial.print(F("[DEBUG] Sent to BT: "));
      Serial.println(cmd);
    }
  }

  // BT201 -> PC Serial Monitor
  while (btSerial.available()) {
    char c = btSerial.read();
    Serial.write(c);
  }
}
