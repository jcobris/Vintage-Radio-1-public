
#include "Bluetooth_Module.h"

BluetoothModule btModule(11, 12); // RX, TX pins for BT201
uint8_t currentFolder = 0;        // Placeholder for folder selection logic

void setup() {
    Serial.begin(115200); // Debug serial
    btModule.begin(57600); // Start BT module serial
    Serial.println("[DEBUG] Booting and configuring BT201...");
    
    // Run setup commands every boot
    btModule.sendInitialCommands();
    
    Serial.print("[DEBUG] Current folder: ");
    Serial.println(currentFolder);
    Serial.println("[DEBUG] Passthrough ready. Type AT commands:");
}

void loop() {
    btModule.passthrough();
}
