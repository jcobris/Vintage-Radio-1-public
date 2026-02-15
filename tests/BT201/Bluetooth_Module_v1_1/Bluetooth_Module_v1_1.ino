
/ Bluetooth_Module.ino
#include "Bluetooth_Module.h"

// Using Arduino D10 as RX (from BT201 TX),
// and D9 as TX (to BT201 RX).
BluetoothModule btModule(10, 9); // RX, TX pins for BT201

uint8_t currentFolder = 0;        // Placeholder for folder selection logic

void setup() {
    // Serial for PC (Serial Monitor) and MP3 module debug: 9600 baud
    Serial.begin(9600);
    delay(200); // small delay to let Serial come up

    // Serial for BT201 module: 57600 baud (as required for BT201)
    btModule.begin(57600);

    Serial.println(F("[DEBUG] Booting and configuring BT201..."));

    // Run setup commands every boot
    btModule.sendInitialCommands();

    Serial.print(F("[DEBUG] Current folder: "));
    Serial.println(currentFolder);
    Serial.println(F("[DEBUG] Passthrough ready. Type AT commands:"));
    Serial.println(F("[DEBUG] Example: AT+QA  (then press Enter)"));
}

void loop() {
    // Simple serial passthrough between PC and BT201
    btModule.passthrough();
}
