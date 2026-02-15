
#ifndef BLUETOOTH_MODULE_H
#define BLUETOOTH_MODULE_H

#include <Arduino.h>
#include <SoftwareSerial.h>

class BluetoothModule {
public:
    BluetoothModule(uint8_t rxPin, uint8_t txPin);
    void begin(long baudRate);
    void sendInitialCommands();
    void passthrough();

private:
    SoftwareSerial btSerial;
    void sendCommand(const String &cmd);
};

#endif
