

void setup() {
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);
}

bool isOn = true;
int freq[2] = {5200, 9000};

void loop() {
  static int currentDelay = 1;
  static bool isTone = false;
  static bool currentFreq = 1;
  static unsigned long lastMillis = millis();
  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis > currentDelay) {
    isTone = !isTone;
    if (isTone && isOn) {
      tone(11, freq[currentFreq]);
      digitalWrite(13, currentFreq);
      currentFreq = !currentFreq;
      currentDelay = 2;
    } else {
      noTone(11);
      currentDelay = 310;
    }
    lastMillis = currentMillis;
  }
}
