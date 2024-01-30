#include <Arduino.h>
#include <Servo.h>
#include <ezButton.h>
#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>

/*
  INPUTS
*/

// Definitions
const int trigger1Pin = 8;              // Inner trigger
const int trigger2Pin = 7;              // Outer trigger
const int selectorPin = 6;              // Rotation selector

// Setup Function
ezButton innerTrigger(trigger1Pin);
ezButton mainTrigger(trigger2Pin);
ezButton selectorSwitch(selectorPin);

void initializeInputs() {
  Serial.println(F("Initializing Inputs"));

  innerTrigger.setDebounceTime(10);
  mainTrigger.setDebounceTime(10);
  selectorSwitch.setDebounceTime(10);
}

/*
  SOUND
*/

// Definitions
DFRobotDFPlayerMini player;

const int defaultVol = 30;

// Track assignments
const int startupSFX = 1;
const int finOpenSFX = 2;
const int finCloseSFX = 3;
const int rotateSFX = 4;
const int errorSFX = 5;
const int fire1SFX = 6;
const int fire2SFX = 7;

// Functions
void playSFX(int track) {
  player.play(track);
}

// Setup Function
void initializeDFPlayer() {
  Serial1.begin(9600);

  Serial.println();
  Serial.println(F("Initializing DFPlayer... (May take 3-5 seconds)"));

  if (!player.begin(Serial1, true, true)) {
    Serial.println(F("DFPlayer initialization failed"));
    Serial.println(F("1. Please recheck the RX/TX connection"));
    Serial.println(F("2. Please recheck the SD card!"));
  }

  Serial.println(F("DFPlayer Mini initialized."));

  player.volume(defaultVol);
}

// DFPlayer logging
void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}


/*
  SERVOS

  This prop uses 2 servo motors, 1 MG90S and 1 DS3218MG, to extend/retract the outer fins and to rotate the whole head respectively.
  The MG90S is driven by a digital PWM pin and powered off of the Arduino 5V pin.
  The DS3218MG is driven by a digital PWM pin and powered directly off the 7.4V power from the battery.
*/

// Fin Servo
Servo finservo;                         // MG90S servo used to actuate retracting fins
const int finServoPin = 9;
const int finExtendedPos = 0;           
const int finRetractedPos = 165;

const int finDelay = 325;               // Approx delay @ 5V

// Fin Servo Functions
void finOpen() {
  playSFX(finOpenSFX);
  finservo.write(finExtendedPos);
  delay(finDelay);
}

void finClose() {
  playSFX(finCloseSFX);
  finservo.write(finRetractedPos);
  delay(finDelay);
}

void finFire() {
  finservo.write(finRetractedPos);
  delay(finDelay/2);
  finservo.write(finExtendedPos);
  delay(finDelay);
}

// Head Servo
Servo headservo;                          // DS3218MG servo used to rotate head
const int headServoPin = 10;
const int headServoMinPulse = 544;        // TODO Check if default pulse widths will work
const int headServoMaxPulse = 2400;
const int headServoMinPos = 0;
const int headServoMaxPos = 90;

const int headDelay = 500;                // TODO Verify head delay @ working voltage (~7V)

// Head Servo Functions
void rotateHead() {
  int currPos = headservo.read();

  finOpen();                              // Extends fins to allow for head rotation

  playSFX(rotateSFX);
  if (currPos == 0) {
    headservo.write(90);
  } else headservo.write(0);

  delay(headDelay);

  finClose();
}

// Setup Function
void initializeServos() {
  Serial.println(F("Initializing servos"));

  finservo.attach(finServoPin);
  headservo.attach(headServoPin, headServoMinPulse, headServoMaxPulse);
}


/*
  LIGHTS
*/
// Definitions
const int laserTransistorPin = 3;

const int ledDataPin = 2;
const int ledNum = 3;

CRGB nozzleLeds[ledNum];

// Functions
void lasersOn() {
  digitalWrite(laserTransistorPin, HIGH);

}

void lasersOff() {
  digitalWrite(laserTransistorPin, LOW);
}

void ledsOn(CRGB color) {
  nozzleLeds[0] = color;
  FastLED.show();
}

void ledsOff() {
  nozzleLeds[0] = CRGB::Black;
  FastLED.show();
}

// Setup Function
void initializeLights() {
  Serial.println(F("Initializing lasers and LEDs"));
  pinMode(laserTransistorPin, OUTPUT);
  
  FastLED.addLeds<NEOPIXEL, ledDataPin>(nozzleLeds, ledNum);
}

/*
  ACTIONS
*/

// State Definitions
bool triggerPrimed = false;

// Functions
void startup() {
  delay(2000);
  playSFX(startupSFX);

  finOpen();
  finClose();

  finOpen();
  headservo.write(0);
  delay(headDelay);
  finClose();

  rotateHead();
  rotateHead();
  if (headservo.read() != 0) {
    rotateHead();
  }
}

void primeTrigger() {
  finOpen();
  ledsOn(CRGB::Cyan);
  lasersOn();
  // triggerPrimed = true;                   // Enables main trigger action
}

void unprimeTrigger() {
  lasersOff();
  ledsOff();
  finClose();
  // triggerPrimed = false;                  // Locks main trigger action
}

void fireTrigger() {                     // Initiates firing action, requires trigger priming
  if (innerTrigger.isPressed()) {
    ledsOn(CRGB::Orange);
    lasersOff();
    playSFX(fire1SFX);
    finFire();
    // Slow fade LEDs
    delay(2000);
    ledsOff();
  } else {
    playSFX(errorSFX);
  }
}

/*
  MAIN
*/
void setup() {
  Serial.begin(115200);

  initializeInputs();
  initializeDFPlayer();
  initializeServos();
  initializeLights();

  // TODO Startup sequence

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Buttons
  innerTrigger.loop();
  mainTrigger.loop();
  selectorSwitch.loop();
  
  // Servo

  // DFPlayer
  // if (myDFPlayer.available()) {
  //   printDetail(myDFPlayer.readType(), myDFPlayer.read());
  // }

  if (innerTrigger.isPressed()) {
    primeTrigger();
  }
  if (innerTrigger.isReleased()) {
    unprimeTrigger();
  }

  if (mainTrigger.isPressed()) {
    fireTrigger();
  }

  if (selectorSwitch.isPressed()) {
    rotateHead();
  }
}
