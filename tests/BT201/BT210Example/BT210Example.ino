#include <BT201.h>

BT201 bt(11, 12); // RX, TX

void onTrackChange(uint16_t index, void* userData) {
    Serial.print("Now playing: ");
    Serial.println(bt.getCurrentTrackName());
}

void setup() {
    Serial.begin(115200);
    bt.begin(57600);
    
    bt.setTrackCallback(onTrackChange);
    bt.setVolume(25);
    bt.playFolder("/01");
}

void loop() {
    bt.processIncoming();
    
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case '1': bt.playFolder("/01"); break;
            case '2': bt.playFolder("/02"); break;
            case '3': bt.playFolder("/03"); break;
        }
    }
}