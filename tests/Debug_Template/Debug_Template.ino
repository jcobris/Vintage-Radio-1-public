  // project description

  // definitions
  // define debuging on or off
  #define DEBUG 0
  // define output pins
  #define blueLED 8
  #define redLED 9

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
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  //set pin initial state
  digitalWrite(blueLED, LOW);
  digitalWrite(redLED, LOW);
    
  debugln("Setup compete.");
}

void loop() {
  // put your main code here, to run repeatedly:
  // put the function calls here
  blinkblueLED();
  blinkredLED();
  
  // debug("Value: ");
  // debugln(b);
}

// Functions
void blinkblueLED(){
  static unsigned long blueMillis = millis();

  if (millis() - blueMillis > 1000) {
    digitalWrite(blueLED, !digitalRead(blueLED));
    blueMillis = millis();
    debug("Blue state: ");
    debugln(digitalRead(blueLED));
  }

}
void blinkredLED(){
  static unsigned long redMillis = millis();

  if (millis() - redMillis > 500) {
    digitalWrite(redLED, !digitalRead(redLED));
    redMillis = millis();
    debug("Red state: ");
    debugln(digitalRead(redLED));
  }

}


// End





