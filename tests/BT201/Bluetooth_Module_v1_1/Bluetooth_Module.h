
// Bluetooth_Module.h
#ifndef BLUETOOTH_MODULE_H
#define BLUETOOTH_MODULE_H

#include <Arduino.h>
#include <SoftwareSerial.h>

class BluetoothModule {
public:
    // rxPin = Arduino pin connected to BT201 TX
    // txPin = Arduino pin connected to BT201 RX
    BluetoothModule(uint8_t rxPin, uint8_t txPin);

    void begin(long baudRate);
    void sendInitialCommands();
    void passthrough();

private:
    SoftwareSerial btSerial;
    void sendCommand(const String &cmd);
};

#endif // BLUETOOTH_MODULE_H
