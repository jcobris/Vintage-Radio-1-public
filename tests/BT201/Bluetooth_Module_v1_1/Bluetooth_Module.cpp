
// Bluetooth_Module.cpp
#include "Bluetooth_Module.h"

BluetoothModule::BluetoothModule(uint8_t rxPin, uint8_t txPin)
    : btSerial(rxPin, txPin) {
}

void BluetoothModule::begin(long baudRate) {
    btSerial.begin(baudRate);
}

void BluetoothModule::sendInitialCommands() {
    // NOTE: This assumes the BT201 is already configured to use 57600 baud
    // and that btSerial.begin(57600) was already called in setup().

    sendCommand("AT+CT04");               // Set baud rate to 57600 (kept as per your working setup)
    delay(50);

    // Keep your working Bluetooth name exactly as-is
    sendCommand("AT+BDO'l Timey Radio");  // Set Bluetooth name
    delay(50);

    sendCommand("AT+CK00");               // Disable auto-switch to Bluetooth
    delay(50);

    sendCommand("AT+B200");               // Disable Bluetooth call functions
    delay(50);

    sendCommand("AT+CA30");               // Volume 100%
    delay(50);

    sendCommand("AT+CN00");               // Disable prompt sounds
    delay(50);

    sendCommand("AT+CQ01");               // EQ Rock
    delay(50);
}

void BluetoothModule::sendCommand(const String &cmd) {
    btSerial.print(cmd);
    btSerial.print("\r\n");
    delay(100); // small gap so module can process the command
}

void BluetoothModule::passthrough() {
    // PC Serial Monitor -> BT201
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');  // read a line until newline
        cmd.trim(); // remove any trailing \r or spaces

        if (cmd.length() > 0) {
            btSerial.print(cmd);
            btSerial.print("\r\n");

            Serial.print(F("[DEBUG] Sent: "));
            Serial.println(cmd);
        }
    }

    // BT201 -> PC Serial Monitor
    while (btSerial.available()) {
        char c = btSerial.read();
        Serial.write(c);
    }
}
