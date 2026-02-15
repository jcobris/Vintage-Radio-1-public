  // project description

  // definitions
  // define debuging on or off
  #define DEBUG 0
  // define output pins
  #define redLED 9
  #define greenLED 10
  #define blueLED 11

  // debug function
  #if DEBUG ==1
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
  #else
  #define debug(x)
  #define debugln(x)
  #endif

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  //set pin modes
  
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  //set pin initial state
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, LOW);
    digitalWrite(blueLED, LOW);

  debugln("Setup compete.");
}

void loop() {
  // put your main code here, to run repeatedly:
  // put the function calls here
  driveredLED();
  drivegreenLED();
  driveblueLED();
  
  // debug("Value: ");
  // debugln(b);
}

// Functions
void driveredLED(){
  static unsigned long redMillis = millis();

  if (millis() - redMillis > 110) {
    digitalWrite(redLED, !digitalRead(redLED));
    redMillis = millis();
    debug("Red state: ");
    debugln(digitalRead(redLED));
  }
}

void drivegreenLED(){
  static unsigned long greenMillis = millis();

  if (millis() - greenMillis > 120) {
    digitalWrite(greenLED, !digitalRead(greenLED));
    greenMillis = millis();
    debug("Green state: ");
    debugln(digitalRead(greenLED));
  }
}
void driveblueLED(){
  static unsigned long blueMillis = millis();

  if (millis() - blueMillis > 130) {
    digitalWrite(blueLED, !digitalRead(blueLED));
    blueMillis = millis();
    debug("Blue state: ");
    debugln(digitalRead(blueLED));
  }
}


// End





