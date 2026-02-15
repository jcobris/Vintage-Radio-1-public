
#include <SoftwareSerial.h>
SoftwareSerial BTSerial(11, 12); // RX, TX for BT201

void setup() {
  Serial.begin(115200);       // Serial Monitor
  BTSerial.begin(57600);      // BT201 at 57600 baud
  Serial.println("[DEBUG] Passthrough ready. Type AT commands:");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    BTSerial.print(cmd);
    BTSerial.print("\r\n");
    Serial.print("[DEBUG] Sent: ");
    Serial.println(cmd);
    delay(50);
  }

  while (BTSerial.available()) {
    char c = BTSerial.read();
    Serial.write(c);
  }
}
