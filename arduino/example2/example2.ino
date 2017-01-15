// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN            6
#define NUMPIXELS      24

#define MAX_INTENSITY 10            
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB | NEO_KHZ800);

int delayval   = 20;
byte current[] = {0, 0, 0};
byte target[]  = {0, 0, 0};

byte clamp(byte b) {
  if (b > MAX_INTENSITY) {
    return MAX_INTENSITY;
  }
  return b;
}

void setAll() {
  for (int i=0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(clamp(current[0]), clamp(current[1]), clamp(current[2])));
    pixels.show();
  }
}

void spinner(int repeat) {
  int offset;
  
  for (int i=0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    pixels.show();
  }
  for (int r = 0; r < repeat; r++) {
    offset = 0;
    while (offset < NUMPIXELS) {
      pixels.setPixelColor((offset - 1 + NUMPIXELS) % NUMPIXELS, pixels.Color(0, 0, 0));
      pixels.setPixelColor((offset - 1 + NUMPIXELS + NUMPIXELS / 2) % NUMPIXELS, pixels.Color(0, 0, 0));
      for (int i=offset; i < offset + NUMPIXELS / 4; i++) {
        pixels.setPixelColor(i % NUMPIXELS, pixels.Color(clamp(current[0]), clamp(current[1]), clamp(current[2])));
        pixels.setPixelColor((i + NUMPIXELS / 2) % NUMPIXELS, pixels.Color(clamp(current[0]), clamp(current[1]), clamp(current[2])));
        pixels.show();
      }
      delay(delayval * 5);
      offset++;
    }
  }
}

void fadeTo() {
  boolean done;
  while (true) {
    done = true;
    for (int i=0; i < 3; i++) {
      if (current[i] < target[i]) {
        current[i]++;
        done = false;
      } else if (current[i] > target[i]) {
        current[i]--;
        done = false;
      }
    }
    if (done) {
      return;
    }
    setAll();
    delay(delayval);
  }
}

void setup() {
  pixels.begin(); // This initializes the NeoPixel library.
}

void loop() {
  target[0] = 0;
  target[1] = 0;
  target[2] = 0;
  fadeTo();
  delay(delayval * 50);

  target[0] = 10;
  target[1] = 10;
  target[2] = 10;
  fadeTo();
  delay(delayval * 50);

  spinner(2);

  target[0] = 0;
  target[1] = 0;
  target[2] = 0;
  fadeTo();
  delay(delayval * 50);
  
//  target[0] = 10;
//  target[1] = 0;
//  target[2] = 0;
//  fadeTo();
//  delay(delayval * 50);
//
//  target[0] = 10;
//  target[1] = 10;
//  target[2] = 0;
//  fadeTo();
//  delay(delayval * 50);
//
//  target[0] = 0;
//  target[1] = 10;
//  target[2] = 0;
//  fadeTo();
//  delay(delayval * 50);
//
//  target[0] = 0;
//  target[1] = 10;
//  target[2] = 10;
//  fadeTo();
//  delay(delayval * 50);
//
//  target[0] = 0;
//  target[1] = 0;
//  target[2] = 10;
//  fadeTo();
//  delay(delayval * 50);
//
//  target[0] = 10;
//  target[1] = 0;
//  target[2] = 10;
//  fadeTo();
//  delay(delayval * 50);
}

