
#include <SoftwareSerial.h>
SoftwareSerial BTSerial(11, 12); // RX, TX for BT201

const int modePin = 2;
bool lastState = HIGH;

void setup() {
  Serial.begin(115200);
  BTSerial.begin(57600);
  pinMode(modePin, INPUT_PULLUP);
  Serial.println("[DEBUG] Ready to switch modes...");
}

void loop() {
  bool currentState = digitalRead(modePin);

  if (currentState != lastState) {
    if (currentState) {
      Serial.println("[DEBUG] Switching to Bluetooth mode...");
      byte cmdBT[] = {0xAA, 0x00, 0x02, 0x00, 0x02};
      BTSerial.write(cmdBT, sizeof(cmdBT));
    } else {
      Serial.println("[DEBUG] Switching to TF card (MP3) mode...");
      byte cmdTF[] = {0xAA, 0x00, 0x02, 0x00, 0x01};
      BTSerial.write(cmdTF, sizeof(cmdTF));
    }
    lastState = currentState;
    delay(500);
  }
}
