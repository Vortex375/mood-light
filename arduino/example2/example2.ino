#include <FastLED.h>
#include <color.h>

#define SERIAL_BAUD_RATE 19200

#define PIN           6
#define NUMPIXELS     240

#define ROTARY_PIN_A  2
#define ROTARY_PIN_B  4
#define SWITCH_PIN    3

#define BLACK         CRGB(0, 0, 0)
#define WHITE         CRGB(255, 255, 255)
#define WHITE_3000K   CRGB(255, 180, 107)
#define WHITE_6500K   CRGB(255, 249, 253)

#define DELAYVAL      250
#define DELAYFADE     4

CRGB leds[NUMPIXELS];
CRGB colorBuffer[NUMPIXELS];

const CRGB PRESETS[] = {UncorrectedTemperature, Candle, Tungsten100W, HighNoonSun, ClearBlueSky, WarmFluorescent, CoolWhiteFluorescent, BlackLightFluorescent};

volatile boolean interrupted;
volatile int encoderChange;
volatile boolean switchTriggered;
uint8_t rotaryValue;
int currentPreset;

void fadeTo() {
  uint8_t *current;
  uint8_t *target;
  boolean done;
  while (true) {
    done = true;
    current = (uint8_t*) leds;
    target = (uint8_t*) colorBuffer;
    
    for (int j = 0; j < NUMPIXELS; j++) {
      for (int i=0; i < sizeof(CRGB); i++) {
        if (current[i] < target[i]) {
          current[i]++;
          done = false;
        } else if (current[i] > target[i]) {
          current[i]--;
          done = false;
        }
      }
      current += sizeof(CRGB);
      target += sizeof(CRGB);
    }
    if (done) {
      return;
    }
    FastLED.show();
    delay(DELAYFADE);
  }
}

void rotate(bool right) {
  CRGB *pxValues = leds;
  if (right) pxValues += NUMPIXELS - 1;
  CRGB first = *pxValues;
  for (int i = 0; i < NUMPIXELS - 1; i++) {
    if (right) {
      pxValues[0] = pxValues[-1];
      pxValues--;
    } else {
      pxValues[0] = pxValues[1];
      pxValues++;
    }
  }
  pxValues[0] = first;

  FastLED.show();
}

void synchronizeSerial() {
  unsigned char input[3];
  while(true) {
    while(Serial.read() != 0) {
      ;
    }
    while(Serial.readBytes(input, 3) != 3) {
      ;
    }
    
    if (input[0] == 0x55 && input[1] == 0xAA && input[2] == 0xFF) {
      return;
    }
  }
}

void doEncoder() {
  if (digitalRead(ROTARY_PIN_A) == digitalRead(ROTARY_PIN_B)) {
    encoderChange++;
  } else {
    encoderChange--;
  }

  interrupted = true;
}

void doSwitch() {
  if (digitalRead(SWITCH_PIN) == HIGH) {
    switchTriggered = true;
  }

  interrupted = true;
}

void setup() {
  pinMode(ROTARY_PIN_A, INPUT_PULLUP);
  pinMode(ROTARY_PIN_B, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), doEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), doSwitch, CHANGE);

  encoderChange = 0;
  switchTriggered = false;
  interrupted = false;
  rotaryValue = 0;
  currentPreset = 0;
  
  FastLED.addLeds<NEOPIXEL, PIN>(leds, NUMPIXELS);

  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  FastLED.setCorrection(Typical8mmPixel);
  FastLED.setTemperature(PRESETS[currentPreset]);

  FastLED.showColor(CRGB::Black);
}

void loop() {
  if (!interrupted) {
    return;
  }
  noInterrupts();

  boolean turnOff = switchTriggered;
  rotaryValue = constrain(rotaryValue + encoderChange * 2, 0, 255);
  encoderChange = 0;
  switchTriggered = false;
  interrupted = false;
  interrupts();

  if (turnOff) {
    currentPreset = (++currentPreset) % 8;
  }
  FastLED.setBrightness(rotaryValue);
  FastLED.setTemperature(PRESETS[currentPreset]);
  FastLED.showColor(CRGB::White);
  
//  synchronizeSerial();
//  while(Serial.available() < sizeof(uint32_t)) {
//    ;
//  }
//  uint32_t color;
//  Serial.readBytes((uint8_t*) &color, sizeof(uint32_t));
//  patternSingle(color);
//  applyInstant();

  
  
//  patternSingle(BLACK);
//  applyInstant();
//  delay(500);
//  patternSingle(WHITE_3000K);
//  fadeTo();
//  delay(5000);
//  patternSingle(WHITE_6500K);
//  fadeTo();
//  delay(5000);
//  patternSingle(BLACK);
//  fadeTo();
//  delay(500);
  
//  for (int i = 0; i < NUMPIXELS; i++) {
//    pixels.setPixelColor(i, i%2 ? BLACK : i%4 ? WHITE_6500K : WHITE_3000K);
//  }
//  pixels.show();
//  while(true) {
//    rotate(true);
//    delay(delayval);
//  }
}

