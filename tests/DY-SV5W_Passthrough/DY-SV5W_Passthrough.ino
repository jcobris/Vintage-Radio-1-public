
#include <Arduino.h>
#include <SoftwareSerial.h>

#define MP3_RX 11  // Arduino RX (connect to MP3 TX)
#define MP3_TX 12  // Arduino TX (connect to MP3 RX)

SoftwareSerial mp3Serial(MP3_RX, MP3_TX);

void setup() {
  Serial.begin(9600);       // Serial Monitor
  mp3Serial.begin(9600);    // DY-SV5W UART baud rate
  Serial.println("MP3 UART Test Ready");
  Serial.println("Type HEX bytes separated by spaces (e.g., AA 04 00) and press Enter");
}

void loop() {
  // Forward data from Serial Monitor to MP3 module
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      Serial.print("Sending: ");
      Serial.println(input);

      // Parse HEX bytes from input
      int start = 0;
      while (start < input.length()) {
        int end = input.indexOf(' ', start);
        if (end == -1) end = input.length();
        String token = input.substring(start, end);
        byte value = (byte) strtol(token.c_str(), NULL, 16);
        mp3Serial.write(value);
        start = end + 1;
      }
    }
  }

  // Echo any response from MP3 module
  while (mp3Serial.available()) {
    byte incoming = mp3Serial.read();
    Serial.print("MP3 -> ");
    Serial.println(incoming, HEX);
  }
}
