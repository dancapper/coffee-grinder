// Coffee Grinder one-shot timer with low power consumption
//
// github.com/dancapper/coffee-grinder
//
// license: use however you like

#include <EEPROM.h>
#include <TimerOne.h>
#include <LowPower.h>

#define versionMajor       1
#define versionMinor       02

#define CONFIG_VERSION "TIM02"
#define CONFIG_START 32

// Type and enum defs

typedef struct {
  char version[6];    // Make sure it's valid config
  unsigned long duration;        // How long per "cup" of coffee, in ms
} configuration_type;

// Default settings

configuration_type CONFIGURATION = {
  CONFIG_VERSION,
  1000                 // 1 second default
};

// Hardware configuration

// I used an Arduino Pro Mini clone, so you may prefer different pinouts

#define debug           false  // Debug over serial, recommend false unless debugging for better performance

#define LOW_POWER_MODE  true  // Low power mode - go to sleep when waiting for button press

#define sensorPin         A0  // select the input pin for the variable resistor, 20k, top to detectPin, bottom gnd
#define detectPin         12  // Pull this pin up to check position of variable resistor
#define MAX_POSITION      228 // reading at maximum, minimum assumed zero
#define NUM_CUPS          12  // cups at maximum position

// Output driver - I used a solid state relay with enough power for the motor, HIGH is on

#define outputPin          11  // select the pin for the output

// Button functions:
// <1s = Run
// >1s = Calibrate
// >3s = Save timer to EEPROM

#define buttonPin          2  // input button, switch to ground, multifunction

// LED indicates current activity

#define ledPin            10  // indicator led
#define ledTimer          200  // ms between led blinks
#define fastLedTimer      50   // ms between fast blinks

// Internal Variables do not alter

unsigned long pressedTime = 0;   // debouncing for button
unsigned long calibrationStartTime = 0;
boolean calibrating = false;
int cupValue = 0;
long timeToRun = -1;
long timeToBlink = -1;
int blinkLed = 0;

void setup() {
  delay(500);              // just a little warmup time, to allow flashing if there's an issue with interrupts etc

  if(debug){
            Serial.begin(57600);
            Serial.println("Coffee Grinder - version " + String(versionMajor) + "." + String(versionMinor)); 
         }

  if(LoadConfig())
    if(debug) Serial.println("Configuration loaded; duration " + String(CONFIGURATION.duration));
  else
    if(debug) Serial.println("Configuration not loaded; using defaults");
  
  cupValue = MAX_POSITION / (NUM_CUPS - 1);
  
  pinMode(outputPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(detectPin, INPUT);
  pinMode(sensorPin, INPUT);
  delay(50);
  attachInterrupt(digitalPinToInterrupt(buttonPin), ButtonPress, FALLING);
  
  OCR0A = 0xAF;            // use the same timer as the millis() function
  TIMSK0 |= _BV(OCIE0A);
  
  if(debug) Serial.println("Ready");
  
  GoToSleep();

}

void loop() {
  
  
}

ISR(TIMER0_COMPA_vect){
  if(timeToRun > 0) timeToRun--;
  else if(timeToRun == 0) {
    Motor(LOW);
    LED(0);
    timeToRun = -1;
    GoToSleep();
  }
  
  if(timeToBlink > 0) timeToBlink--;
  else if(timeToBlink == 0) {
    LED(0);
    timeToBlink = -1;
    GoToSleep();
  }
  
  if(blinkLed == 1 && millis() % ledTimer == 0) ToggleLED();
  if(blinkLed == 2 && millis() % fastLedTimer == 0) ToggleLED();
}

void GoToSleep() {
  // Low power enhancement
      
  if(LOW_POWER_MODE) {
    if(debug) {
      Serial.println("Shh... don't fight it (entering sleep mode)");
      Serial.flush();
    }
    
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}

void ToggleLED() {
  if(digitalRead(ledPin) == HIGH) digitalWrite(ledPin, LOW);
  else digitalWrite(ledPin, HIGH);
}

void ButtonPress() {
  if(debug) 
    Serial.println("Button pressed at " + String(millis()));
  if(digitalRead(buttonPin) == LOW) {
    pressedTime = millis();
    attachInterrupt(digitalPinToInterrupt(buttonPin), ButtonRelease, RISING);
  }
}

void ButtonRelease() {
  if(debug)
    Serial.println("Button released at " + String(millis()));
  if(digitalRead(buttonPin) == HIGH) {
    int duration = millis() - pressedTime;
    if(calibrating && duration > 30) {
      // End of calibration
      EndCalibration();
    } else if(duration > 30 && duration < 1000) {
      // Trigger one-shot timer
      OneShot();
    } else if(duration >=1000 && duration < 3000) {
      // Calibration run
      StartCalibration();
    } else if(duration >= 3000){
      // Save config
      SaveConfig();
    }
      attachInterrupt(digitalPinToInterrupt(buttonPin), ButtonPress, FALLING);
  }
}

void StartCalibration() {
  calibrationStartTime = millis();
  LED(2);
  Motor(HIGH);
  calibrating = true;
  if(debug) Serial.println("Calibration started at " + String(calibrationStartTime));
}

void EndCalibration() {
 LED(0);
 Motor(LOW);
 calibrating = false;
 unsigned long runtime = pressedTime - calibrationStartTime;
 if(debug) Serial.println("Calibration end time is " + String(pressedTime) + " - calculated time is " + String(runtime));
 CONFIGURATION.duration = runtime;
 GoToSleep();
}

void OneShot() {
  if(timeToRun > 0){
    Motor(LOW);
    LED(0);
    timeToRun = -1;
  } else {
    LED(1);
    int cups = ReadDial();
    unsigned long runtime = cups * CONFIGURATION.duration;
    if(debug) Serial.println("Runtime will be " + String(runtime));
    Motor(HIGH);
    timeToRun = runtime;
  }
}

int ReadDial() {
  pinMode(detectPin, INPUT_PULLUP);
  int dialPosition = analogRead(sensorPin);
  if(debug) Serial.println("Read dial as " + String(dialPosition));
  pinMode(detectPin, INPUT);
  int cups = ceil(dialPosition / cupValue) + 1;
  if(debug) Serial.println("Calculated cups as " + String(cups));
  return cups;  
}

void LED(int ledState) {
  if(ledState==1) {
    digitalWrite(ledPin, HIGH);
    blinkLed = 0;
  } else if(ledState==0) {
    digitalWrite(ledPin, LOW);
    blinkLed = 0;
  } else if(ledState==2) {
    blinkLed = 1;
  } else if(ledState==3) {
    blinkLed = 2;
    timeToBlink = 2000;
  }
}

void Motor(boolean outputState) {
  if(debug) Serial.println("Set motor to " + String(outputState));
  digitalWrite(outputPin, outputState);
}

int LoadConfig() {
  // is it correct?
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2] &&
      EEPROM.read(CONFIG_START + 3) == CONFIG_VERSION[3] &&
      EEPROM.read(CONFIG_START + 4) == CONFIG_VERSION[4]){
 
  // load (overwrite) the local configuration struct
    for (unsigned int i=0; i<sizeof(CONFIGURATION); i++){
      *((char*)&CONFIGURATION + i) = EEPROM.read(CONFIG_START + i);
    }
    return 1; // return 1 if config loaded 
  }
  return 0; // return 0 if config NOT loaded
}

void SaveConfig() {
  LED(3);
  if(debug) Serial.println("Writing configuration; duration is " + String(CONFIGURATION.duration));
  for (unsigned int i=0; i<sizeof(CONFIGURATION); i++)
    EEPROM.write(CONFIG_START + i, *((char*)&CONFIGURATION + i));
}
