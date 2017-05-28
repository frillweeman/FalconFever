#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LEDStrip {
public:
  enum Signal {
    left,
    right,
    hazard,
    police,
    none
  };
  
private:

  /*********************
   * PRIVATE VARIABLES *
   *********************/

  // Adafruit LED Strip object
  Adafruit_NeoPixel pixels;

  // Holds start and end Rangees for LED groups
  struct Range { int start, end; }
    leftSignal, rightSignal, brakeLight,
    l1, l2, r1, r2;
  
  int pin;
  int fullWidth;
  bool isBraking;
  
  // Turn signal variables
  Signal signal;
  unsigned long tSignalInterval;
  
  // Saved colors
  const uint32_t tSignalOn = pixels.Color(255, 165, 0);
  const uint32_t off = pixels.Color(0, 0, 0);
  const uint32_t brakeOn = pixels.Color(255, 0, 0);
  const uint32_t red = pixels.Color(255, 0, 0);
  const uint32_t blue = pixels.Color(0, 0, 255);
  const uint32_t pink = pixels.Color(255,20,147);
  
  // Light Pattern Template
  struct LightPattern {
    int length;
    
    // Data
    int delay;
    int longDelay;
    uint32_t colorData[4][4];
    int order[22];
  };
  
  
  // 1. Quad
  LightPattern alternate = {10, 60, 300, {
        {red, off, blue, off},
        {off, red, off, blue},
        {off, off, off, off}
    }, {0,2,0,2,69,1,2,1,2,69}};
  
  // 2. Quint
  LightPattern flash3 = {20, 40, 100, {
        {red, red, off, off},
        {off, off, blue, blue},
        {off, off, off, off}
    }, {0,2,0,2,0,2,0,2,0,2,1,2,1,2,1,2,1,2,1,2}};

  // 3. Slow Alternate
  LightPattern sides = {2, 300, 60, {
    {red, red, off, off},
    {off, off, blue, blue}
  }, {0,1}};

    LightPattern pinkPattern = {20, 40, 100, {
        {pink, pink, off, off},
        {off, off, pink, pink},
        {off, off, off, off}
    }, {0,2,0,2,0,2,0,2,0,2,1,2,1,2,1,2,1,2,1,2}};
  
  LightPattern *policeMode[4] = {&alternate, &flash3, &sides, &pinkPattern};
  
  int currentLightPattern;
  int cyclesCounter;
  
  
  /*********************
   * PRIVATE FUNCTIONS *
   *********************/
  
  // Set a single color for a range of lights
  void setColorRange(Range range, uint32_t color) {
    for (int i = range.start; i <= range.end; i++) {
      pixels.setPixelColor(i, color);
    }
    pixels.show();
  }
  
  // Toggle appropriate signal when called
  void toggleSignal(Range currentSignal, bool wasOnLast) {
    setColorRange(currentSignal, (wasOnLast ? off : tSignalOn));
  }
  
public:

  /********************
   * PUBLIC FUNCTIONS *
   ********************/
   
  LEDStrip(int _pin, int _fullWidth, int signalWidth, unsigned long _tSignalInterval) 
    : pin(_pin),
      fullWidth(_fullWidth),
      signal(none),
      isBraking(false),
      tSignalInterval(_tSignalInterval),
      pixels(_fullWidth, _pin, NEO_GRB + NEO_KHZ800),
      currentLightPattern(0)
  {
    // Initialize LED strip
    pixels.begin();  

    // Set start and end Ranges for LED groups
    updateGroupWidths(signalWidth);
    pixels.show();
  }
  
  // --------------------------------------------------------------------------------

  // Call this every iteration of loop()
  void update() { 
    static Signal lastSignal = signal;
    static unsigned long lastBlink = 0;
    
    static bool wasOnLast = false;
    static unsigned long currentInterval = tSignalInterval;
    
    // Turn Signal
    if (signal != none) {

      if (signal == police) {
        if (policeMode[currentLightPattern]->order[cyclesCounter] == 69) {
          currentInterval = policeMode[currentLightPattern]->longDelay;
        } else {
          currentInterval = policeMode[currentLightPattern]->delay;
        }
      }
      
      if (signal != lastSignal) {
        // Make sure last signal is turned off before continuing
        // True argument turns light off
        toggleSignal(leftSignal, true);
        toggleSignal(rightSignal, true);
        
        // Flash on first call of update()
        wasOnLast = false;
        
        if (signal != police) {
          currentInterval = tSignalInterval;
        }
      }
      
      unsigned long currentMillis = millis();
      
      // Periodic State Change
      if (currentMillis - lastBlink >= currentInterval) {
        //Serial.print(currentMillis - lastBlink); Serial.print(" > "); Serial.println(currentInterval);
        
        switch (signal) {
        case left:
          toggleSignal(leftSignal, wasOnLast);
          break;
        case right:
          toggleSignal(rightSignal, wasOnLast);
          break;
        case hazard:
          toggleSignal(leftSignal, wasOnLast);
          toggleSignal(rightSignal, wasOnLast);
          break;
        case police:
          if (cyclesCounter == 69) {
            cyclesCounter++;
          } else if (cyclesCounter >= policeMode[currentLightPattern]->length) {
            cyclesCounter = 0;
          }
          
          setColorRange(l1, policeMode[currentLightPattern]->colorData[policeMode[currentLightPattern]->order[cyclesCounter]][0]);
          setColorRange(l2, policeMode[currentLightPattern]->colorData[policeMode[currentLightPattern]->order[cyclesCounter]][1]);
          setColorRange(r1, policeMode[currentLightPattern]->colorData[policeMode[currentLightPattern]->order[cyclesCounter]][2]);
          setColorRange(r2, policeMode[currentLightPattern]->colorData[policeMode[currentLightPattern]->order[cyclesCounter]][3]);
          
          cyclesCounter++;
          
          break;
        default:
          break;
        }
        
        // Update variables
        wasOnLast = !wasOnLast;
        lastBlink = currentMillis;
      }
    } 
    
    // Falling edge -> blank all signal LEDs
    else if (lastSignal != none) {
      // true argument guarantees lights are off
      toggleSignal(leftSignal, true);
      toggleSignal(rightSignal, true);
    }
    
    lastSignal = signal;
  }
  
  // --------------------------------------------------------------------------------

  // Call this to begin and end signaling, not repeatedly
  void tsignal(Signal _signal) {
    signal = _signal;
  }
  
  // --------------------------------------------------------------------------------
  
  // Call this to begin and end braking, not repeatedly
  void brake(bool on) {
    setColorRange(brakeLight, (on ? brakeOn : off));
    isBraking = on;
  }
  
  // --------------------------------------------------------------------------------
  
  void updateGroupWidths(int signalWidth) {
    
    int brakeWidth = fullWidth - (signalWidth * 2);
    
    leftSignal.start = 0;
    leftSignal.end = signalWidth - 1;
    
    brakeLight.start = signalWidth;
    brakeLight.end = signalWidth + brakeWidth - 1;
    
    rightSignal.start = brakeLight.end + 1;
    rightSignal.end = fullWidth;
    
    l1.start = leftSignal.start;
    l1.end = leftSignal.end / 2;
    
    l2.start = l1.end + 1;
    l2.end = leftSignal.end;
    
    r1.start = rightSignal.start;
    r1.end = rightSignal.start + (rightSignal.end - rightSignal.start) / 2 - 1;
    
    r2.start = r1.end + 1;
    r2.end = rightSignal.end;

    // update brake light when width changes
    brake(isBraking);
  }
  
  // --------------------------------------------------------------------------------

  void updateInterval(int newInterval) {
    tSignalInterval = newInterval;
  }
  
  void policeLights(int mode) {
    switch (mode) {
      case -1:
        signal = none;
        break;
      default:
        signal = police;
        currentLightPattern = mode;
        cyclesCounter = 0;
        break;
    }
  }
};