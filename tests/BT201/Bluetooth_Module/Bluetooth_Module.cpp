
#include "Bluetooth_Module.h"

BluetoothModule::BluetoothModule(uint8_t rxPin, uint8_t txPin) : btSerial(rxPin, txPin) {}

void BluetoothModule::begin(long baudRate) {
    btSerial.begin(baudRate);
}

void BluetoothModule::sendInitialCommands() {
    sendCommand("AT+CT04"); // Set baud rate to 57600
    sendCommand("AT+BDO'l Timey Radio"); // Set Bluetooth name
    sendCommand("AT+CK00"); // Disable auto-switch to Bluetooth
    sendCommand("AT+B200"); // Disable Bluetooth call functions
    sendCommand("AT+CA15"); // Volume 50%
    sendCommand("AT+CN00"); // Disable prompt sounds
    sendCommand("AT+CQ01"); // EQ Rock



}

void BluetoothModule::sendCommand(const String &cmd) {
    btSerial.print(cmd);
    btSerial.print("\r\n");
    delay(100);
}

void BluetoothModule::passthrough() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        btSerial.print(cmd);
        btSerial.print("\r\n");
        Serial.print("[DEBUG] Sent: ");
        Serial.println(cmd);
        delay(50);
    }

    while (btSerial.available()) {
        char c = btSerial.read();
        Serial.write(c);
    }
}
