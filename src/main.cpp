#include <Arduino.h>
#include <Servo.h>
#include <ezButton.h>
#include <FastLED.h>
#include <DFPlayerMini_Fast.h>


/*
  GLOBAL VARIABLES
*/

unsigned long currMillis = 0;

/*
  INPUTS
*/

// Definitions
const uint8_t trigger1Pin = 8;              // Inner trigger
const uint8_t trigger2Pin = 7;              // Outer trigger
const uint8_t selectorPin = 6;              // Rotation selector

// Setup Function
// ezButton innerTrigger(trigger1Pin);
// ezButton mainTrigger(trigger2Pin);
// ezButton selectorSwitch(selectorPin);

void initializeInputs() {
  Serial.println(F("Initializing Inputs"));

  // innerTrigger.setDebounceTime(10);
  // mainTrigger.setDebounceTime(10);
  // selectorSwitch.setDebounceTime(500);

  pinMode(trigger1Pin, INPUT_PULLUP);
  pinMode(trigger2Pin, INPUT_PULLUP);
  pinMode(selectorPin, INPUT_PULLUP);
}

/*
  SOUND
*/

// Definitions
DFPlayerMini_Fast player;

const uint8_t defaultVol = 30;

// Track assignments
const uint8_t startupSFX = 1;
const uint8_t finOpenSFX = 2;
const uint8_t finCloseSFX = 3;
const uint8_t rotateSFX = 4;
const uint8_t errorSFX = 5;
const uint8_t fire1SFX = 6;
const uint8_t fire2SFX = 7;

// Functions
void playSFX(int track) {
  player.play(track);
}

// Setup Function
void initializeDFPlayer() {
  Serial1.begin(9600);
  Serial.println(F("Initializing DFPlayer... (May take 3-5 seconds)"));
  player.begin(Serial1, true);
  Serial.println(F("DFPlayer Mini initialized."));

  player.volume(defaultVol);
}

/*
  SERVOS

  This prop uses 2 servo motors, 1 MG90S and 1 DS3218MG, to extend/retract the outer fins and to rotate the whole head respectively.
  The MG90S is driven by a digital PWM pin and powered off of the Arduino 5V pin.
  The DS3218MG is driven by a digital PWM pin and powered directly off the 7.4V power from the battery.
*/

// Fin Servo
Servo finservo;                         // MG90S servo used to actuate retracting fins
const uint8_t finServoPin = 9;
const uint8_t finExtendedPos = 0;
const uint8_t finRetractedPos = 165;

const uint16_t finInterval = 325;               // Approx delay @ 5V

uint8_t finPos = finRetractedPos;
unsigned long lastFinMillis = 0;
bool finMoving = false;

// Fin Servo Functions
void finOpen() {
  if (!finMoving && finPos != finExtendedPos) {
      finservo.write(finExtendedPos);
      finPos = finExtendedPos;
      finMoving = true;
      lastFinMillis = currMillis;
    }
  if (finMoving) {
    if (currMillis >= lastFinMillis + finInterval) {
      finMoving = false;
    }
  }
}

void finClose() {
  if (!finMoving && finPos != finRetractedPos) {
      finservo.write(finRetractedPos);
      finPos = finRetractedPos;
      finMoving = true;
      lastFinMillis = currMillis;
    }
  if (finMoving) {
    if (currMillis >= lastFinMillis + finInterval) {
      finMoving = false;
    }
  }
}

// Head Servo
Servo headservo;                          // DS3218MG servo used to rotate head
const uint8_t headServoPin = 10;
const uint16_t headServoMinPulse = 544;        // TODO Check if default pulse widths will work
const uint16_t headServoMaxPulse = 2400;
const uint8_t headServoMinPos = 3;
const uint8_t headServoMaxPos = 67;

const uint16_t headInterval = 500;                // TODO Verify head delay @ working voltage (~7V)

uint8_t headPos = headServoMinPos;
unsigned long lastHeadMillis = 0;
bool headMoving = false;

void rotateHead() {
  int currPos = headservo.read();

  finOpen();                              // Extends fins to allow for head rotation

  playSFX(rotateSFX);
  if (currPos == headServoMinPos) {
    headservo.write(headServoMaxPos);
  } else headservo.write(headServoMinPos);

  delay(headInterval);

  finClose();
}

// Setup Function
void initializeServos() {
  Serial.println(F("Initializing servos"));

  finservo.attach(finServoPin);
  headservo.attach(headServoPin, headServoMinPulse, headServoMaxPulse);

  finservo.write(finExtendedPos);           // Sets fins and head to home positions
  delay(finInterval);
  headservo.write(headServoMinPos);
  delay(headInterval);
  finservo.write(finRetractedPos);
  delay(finInterval);
}


/*
  LIGHTS
*/
// Declarations
const uint8_t laserTransistorPin = 3;

const uint8_t ledDataPin = 2;
const uint8_t ledNum = 3;

unsigned long ledRefreshInterval = 10000;

CRGB nozzleLeds[ledNum];

// Variables
bool ledStatus = false;
unsigned long lastLEDRefreshMillis = 0;

// Functions
void lasersOn() {
  digitalWrite(laserTransistorPin, HIGH);
}

void lasersOff() {
  digitalWrite(laserTransistorPin, LOW);
}

void ledsOn(CRGB newColor) {
  if (!ledStatus) {
    nozzleLeds[0] = newColor;
    FastLED.show();
    ledStatus = true;
  }
}

void ledsOff() {
  if (ledStatus) {
    nozzleLeds[0] = CRGB::Black;
    FastLED.show();
    ledStatus = false;
  }
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
  delay(headInterval);
  finClose();

  rotateHead();
  rotateHead();
}

void primeTrigger() {
  finOpen();
  ledsOn(CRGB::Cyan);
  // lasersOn();
  // triggerPrimed = true;                   // Enables main trigger action
}

void unprimeTrigger() {
  // lasersOff();
  ledsOff();
  finClose();
  // triggerPrimed = false;                  // Locks main trigger action
}

void fireTrigger() {                     // Initiates firing action, requires trigger priming
  // if (innerTrigger.isPressed()) {
  //   ledsOn(CRGB::Orange);
  //   lasersOff();
  //   playSFX(fire1SFX);
  //   // finFire();
  //   // Slow fade LEDs
  //   delay(2000);
  //   ledsOff();
  // } else {
  //   playSFX(errorSFX);
  // }
}

/*
  MAIN
*/
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  initializeInputs();
  // initializeDFPlayer();
  initializeServos();
  initializeLights();

  // startup();
}

void loop() {
  currMillis = millis();

  if (ledStatus) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if (!ledStatus) {
        digitalWrite(LED_BUILTIN, LOW);
  }

  byte trigger1State = digitalRead(trigger1Pin);
  // byte trigger2State = digitalRead(trigger2Pin);
  // byte selectorState = digitalRead(selectorPin);
  if (trigger1State == LOW) {
    primeTrigger();
  }
  if (trigger1State == HIGH) {
    unprimeTrigger();
  }

  // Buttons
  // innerTrigger.loop();
  // mainTrigger.loop();
  // selectorSwitch.loop();

  // if (innerTrigger.isPressed()) {
  //   primeTrigger();
  // }
  // if (innerTrigger.isReleased()) {
  //   unprimeTrigger();
  // }
  // if (mainTrigger.isPressed()) {
  //   fireTrigger();
  // }
  // if (selectorSwitch.isPressed()) {
  //   rotateHead();
  // }
}