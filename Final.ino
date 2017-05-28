/*
 * Falcon Fever Car Software 2017
 * (c) 2017 Will Freeman
 */

// --- CONFIGURATION

  // Pins

    // Digital
    #define IGNITION_PIN 2
    #define MOTOR_CONTROLLER_PIN 3
    #define BRAKE_LIGHT_SWITCH_PIN 4
    #define LED_STRIP_PIN 5
    #define HALL_EFFECT_PIN 6

    const int HALL_EFFECT_INT_PIN = digitalPinToInterrupt(HALL_EFFECT_PIN);

    // Analog
    #define THROTTLE_SPEED_PIN 0

    #define SPEED_GAUGE V9
  
  // Settings
    #define TIRE_DIAMETER 18 // inches
    #define SPEED_UPDATE_INTERVAL 0.5e+6 // update interval (in microseconds) of speed display
  
    // Comment this line out to disable Bluetooth
    #define USING_BLE

    #define EQ_ZERO 5.0e+6 // period (in microseconds) at which speed is considered zero

    #define LED_STRIP_FULL_WIDTH 24 // Don't change this unless you really want to fuck up the lights
    #define LED_STRIP_INITIAL_SIGNAL_WIDTH 8
    #define LED_STRIP_INITIAL_SIGNAL_INTERVAL 400 // milliseconds

// --- END CONFIGURATION


// Libraries
#include <BlynkSimpleCurieBLE.h>
#include <CurieBLE.h>

#include "LEDStrip.cpp"
#include "MotorController.cpp"

// Objects
LEDStrip strip(LED_STRIP_PIN, LED_STRIP_FULL_WIDTH, LED_STRIP_INITIAL_SIGNAL_WIDTH, LED_STRIP_INITIAL_SIGNAL_INTERVAL);
MotorController motor(MOTOR_CONTROLLER_PIN, IGNITION_PIN);

// Globals
volatile int halfRevs = 0;

#ifdef USING_BLE
BLEPeripheral blePeripheral;
#endif

void setup() {

  #ifdef USING_BLE
  blePeripheral.setLocalName("GPCar");
  blePeripheral.setDeviceName("GPCar");
  blePeripheral.setAppearance(384);

  Blynk.begin(blePeripheral, "");
  blePeripheral.begin();
  #endif

  Serial.begin(9600);

  pinMode(BRAKE_LIGHT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(HALL_EFFECT_PIN, INPUT);
  
}

void loop() {

  // Speed
  int speed = analogRead(THROTTLE_SPEED_PIN);
  Serial.println(speed);

  if (speed < 795) {
    speed = map(speed, 800, 0, 0, 50);
  } else if (speed < 805) {
    speed = 0;
  } else {
    speed = map(speed, 800, 1023, 0, 100);
  }
  // End speed


  // Brakes
  static bool wasBraking = false;
  bool isBraking = !digitalRead(BRAKE_LIGHT_SWITCH_PIN);
  
  if (isBraking) {
    speed = 0;
    if (!wasBraking) strip.brake(true);
  } else if (!isBraking && wasBraking) {
    strip.brake(false);
  }

  wasBraking = isBraking;
  // End Brakes

  
  // Run Motor
  motor.setSpeed(speed);


  // Speedometer
  static unsigned long lastMicros = micros() - EQ_ZERO;

  unsigned long currentMicros = micros();
  static double rpm = 0.0;

  static double lastSpeedMPH = -1.0;

  if (currentMicros - lastMicros >= SPEED_UPDATE_INTERVAL) {
  
    if (halfRevs > 2) {
      disableInt();
      rpm = halfRevs / 2.0 // revolutions per
      / ((currentMicros - lastMicros) * 1.0e+6); // minute
      halfRevs = 0;
      lastMicros = currentMicros;
      enableInt();
    } else {
      if (currentMicros - lastMicros >= EQ_ZERO) {
        rpm = 0;
        halfRevs = 0;
        lastMicros = currentMicros;
      }
    }
  
    double speedMPH = (rpm * TIRE_DIAMETER * PI) * 60.0 / 63360.0;
    if (fabs(speedMPH - lastSpeedMPH) >= 0.2) {
      Blynk.virtualWrite(SPEED_GAUGE, speedMPH);
    }
    lastSpeedMPH = speedMPH;
  }
  
  // End Speedometer
  
  // Background Tasks
  strip.update();
  
  #ifdef USING_BLE
  Blynk.run();
  blePeripheral.poll();
  #endif
}

void enableInt() {
  attachInterrupt(HALL_EFFECT_INT_PIN, increment, RISING);
}

void disableInt() {
  detachInterrupt(HALL_EFFECT_INT_PIN);
}

void increment() {
  halfRevs++;
}

#ifdef USING_BLE

// Buttons
#define BTN_LEFT_BLINKER V0
#define BTN_RIGHT_BLINKER V1
#define BTN_HAZARDS V2
#define BTN_QUINT V3
#define BTN_ALTERNATE V4
#define BTN_PINK V5

// Sliders
#define UPDATE_GROUP_WIDTHS V6
#define UPDATE_INTERVAL V7
#define SPEED V8

const int buttonsInUseLength = 6;
const int buttonsInUse[buttonsInUseLength] = {BTN_LEFT_BLINKER, 
                                              BTN_RIGHT_BLINKER, 
                                              BTN_HAZARDS, 
                                              BTN_QUINT, 
                                              BTN_ALTERNATE, 
                                              BTN_PINK};

BLYNK_WRITE(BTN_RIGHT_BLINKER) {
  turnOffOthers(BTN_RIGHT_BLINKER);
  strip.tsignal(param.asInt() ? LEDStrip::left : LEDStrip::none);
}

BLYNK_WRITE(BTN_LEFT_BLINKER) {
  turnOffOthers(BTN_LEFT_BLINKER);
  strip.tsignal(param.asInt() ? LEDStrip::right : LEDStrip::none);
}

BLYNK_WRITE(BTN_HAZARDS) {
  turnOffOthers(BTN_HAZARDS);
  strip.tsignal(param.asInt() ? LEDStrip::hazard : LEDStrip::none);
}

BLYNK_WRITE(UPDATE_INTERVAL) {
  strip.updateInterval(param.asInt());
}

BLYNK_WRITE(UPDATE_GROUP_WIDTHS) {
  strip.updateGroupWidths(param.asInt());
}

BLYNK_WRITE(SPEED) {
  motor.setSpeed(param.asInt());
}

BLYNK_WRITE(BTN_QUINT) {
  turnOffOthers(BTN_QUINT);
  strip.policeLights((param.asInt() ? 1 : -1));
}

BLYNK_WRITE(BTN_ALTERNATE) {
  turnOffOthers(BTN_ALTERNATE);
  strip.policeLights((param.asInt() ? 2 : -1));
}

BLYNK_WRITE(BTN_PINK) {
  turnOffOthers(BTN_PINK);
  strip.policeLights((param.asInt() ? 3 : -1));
}

void turnOffOthers(int currentVirtualPin) {
  for (int i = 0; i < buttonsInUseLength; i++) {
    if (buttonsInUse[i] != currentVirtualPin) {
      Blynk.virtualWrite(buttonsInUse[i], 0);
    }
  }
}

#endif

