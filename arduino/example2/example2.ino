// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define SERIAL_BAUD_RATE 19200

#define PIN           6
#define NUMPIXELS     240

#define BLACK         pixels.Color(0, 0, 0)
#define WHITE         pixels.Color(255, 255, 255)
#define WHITE_3000K   pixels.Color(255, 180, 107)
#define WHITE_6500K   pixels.Color(255, 249, 253)

#define DELAYVAL      250
#define DELAYFADE     5

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB | NEO_KHZ800);

// for direct pixel access in NEO_GRB mode
typedef struct {uint8_t g; uint8_t r; uint8_t b;} pixel_t;

pixel_t colorBuffer[NUMPIXELS];

void patternSingle(uint32_t color) {
  uint8_t *c = (uint8_t*) &color;
  colorBuffer[0].r = c[3];
  colorBuffer[0].g = c[2];
  colorBuffer[0].b = c[1];
  memcpy(colorBuffer+1, colorBuffer, sizeof(colorBuffer) - sizeof(pixel_t));
}

void applyInstant() {
  memcpy(pixels.getPixels(), colorBuffer, sizeof(colorBuffer));
  pixels.show();
}

void fadeTo() {
  uint8_t *current;
  uint8_t *target;
  boolean done;
  while (true) {
    done = true;
    current = pixels.getPixels();
    target = (uint8_t*) colorBuffer;
    
    for (int j = 0; j < NUMPIXELS; j++) {
      for (int i=0; i < sizeof(pixel_t); i++) {
        if (current[i] < target[i]) {
          current[i]++;
          done = false;
        } else if (current[i] > target[i]) {
          current[i]--;
          done = false;
        }
      }
      current += sizeof(pixel_t);
      target += sizeof(pixel_t);
    }
    if (done) {
      return;
    }
    pixels.show();
    delay(DELAYFADE);
  }
}

void rotate(bool right) {
  pixel_t *pxValues = (pixel_t*) pixels.getPixels();
  if (right) pxValues += NUMPIXELS - 1;
  pixel_t first = *pxValues;
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

  pixels.show();
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

void setup() {
  pixels.begin(); // This initializes the NeoPixel library.

  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  patternSingle(BLACK);
  applyInstant();
}

void loop() {
  synchronizeSerial();
  while(Serial.available() < sizeof(uint32_t)) {
    ;
  }
  uint32_t color;
  Serial.readBytes((uint8_t*) &color, sizeof(uint32_t));
  patternSingle(color);
  applyInstant();
  
  
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

