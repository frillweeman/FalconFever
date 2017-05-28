#include <Arduino.h>

class MotorController {
private:
  const static int PWM_MIN = (255 * (1.0/5.0));
  const static int PWM_MAX = (255 * (4.0/5.0));

  int pwmPin, ignitionPin;
  
public:
  MotorController(int pwm, int ignition)
  : pwmPin(pwm), 
    ignitionPin(ignition)
  {
    pinMode(pwmPin, OUTPUT);
    pinMode(ignitionPin, OUTPUT);
  }

  void setSpeed(int speed) {
    analogWrite(pwmPin, map(speed, 0, 100, PWM_MIN, PWM_MAX));
    if (speed == 0) {
      digitalWrite(ignitionPin, 0);
    } else {
      digitalWrite(ignitionPin, 1);
    }
  }
  
};