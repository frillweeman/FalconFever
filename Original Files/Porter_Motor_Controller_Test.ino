// Falcon Fever Car Software
// 2017

// Pins
#define BRAKE_PIN 2
#define MOTOR_CONTROLLER_PIN 9

// Motor controller response range is 1V to 4V 
// (1/5 to 4/5 duty cycle)
#define PWM_MIN (255 * (1.0/5.0))
#define PWM_MAX (255 * (4.0/5.0))

// BLE Libraries
#include <BlynkSimpleCurieBLE.h>
#include <CurieBLE.h>

// OLED Display Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

BLEPeripheral  blePeripheral;
Adafruit_SSD1306 display(4);

int speed = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);

  blePeripheral.setLocalName("GPCar");
  blePeripheral.setDeviceName("GPCar");
  blePeripheral.setAppearance(384);

  Blynk.begin(blePeripheral, "");

  blePeripheral.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  display.setTextSize(2);
  display.setTextColor(WHITE);

  Serial.println("Waiting for connections...");
}

void loop() {
  static unsigned long lastMillis = millis() - 1000;
  static int lastSpeed = -1;
  
  Blynk.run();
  blePeripheral.poll();

  unsigned long currentMillis = millis();

  // Update OLED at 1 Hz
  if (currentMillis - lastMillis >= 500 && speed != lastSpeed) {
    display.clearDisplay();
    display.setCursor(0,30);
    display.print(speed); display.println("%");

    display.fillRect(0, 50, map(speed, 0, 100, 0, 128), 10, WHITE);

    display.setCursor(0,0);
    if (blePeripheral.connected()) {
      display.print("Connected");
    } else {
      display.print("Waiting...");
    }

    // Update Display
    display.display();

    lastMillis = currentMillis;
    lastSpeed = speed;
  }
}

void setSpeed(int speed) {
  analogWrite(MOTOR_CONTROLLER_PIN, map(speed, 0, 100, PWM_MIN, PWM_MAX));
}

BLYNK_WRITE(V0) {
  speed = param.asInt();
  setSpeed(speed);
}

