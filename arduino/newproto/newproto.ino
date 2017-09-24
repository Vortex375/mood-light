#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define SERIAL_BAUD_RATE 9600


/* ===================================================
 *  Pixel setup
 * =================================================== */

#define PIN           6
#define NUMPIXELS     240

#define BLACK         pixels.Color(0, 0, 0)
#define WHITE         pixels.Color(255, 255, 255)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB | NEO_KHZ800);
typedef struct {uint8_t g; uint8_t r; uint8_t b;} pixel_t;
pixel_t colorBuffer[NUMPIXELS];


/* ===================================================
 *  Protocol setup
 * =================================================== */

#define MEMBER_ADDRESS           1

#define PROTO_MARKER_0           0x00
#define PROTO_MARKER_1           0x55
#define PROTO_MARKER_2           0xAA
#define PROTO_MARKER_3           0xFF

#define PROTO_ADDR_BROADCAST     255
#define PROTO_CMD_NOOP           0
#define PROTO_CMD_PATTERN_SIMPLE 1
#define PROTO_CMD_ROTATE         2
#define PROTO_MOD_NONE           0
#define PROTO_MOD_FADE           (1 << 4)
#define PROTO_FLAG_REPEAT        1
#define PROTO_FLAG_LONG_PAYLOAD  (1 << 7)

typedef struct protocol_header_s {

  byte startMarker[4];
  byte memberAddress;
  byte command;
  byte flags;
  union {
    uint16_t payloadLength; /* when using LONG_PAYLOAD */
    uint32_t payload;       /* otherwise (4-byte payload) */
  };
  byte checksum;
  
} protocol_header_t;

protocol_header_t currentHeader;
byte protocolRingBuffer[sizeof(protocol_header_t)];
short protocolBufferOffset;
byte lastByte; /* the last byte that was read from serial */

/* command state */
bool newCommand;
bool needFade;
bool needShow;
short longPayloadLength;
short longPayloadIndex;


/* ===================================================
 *  Pixel methods
 * =================================================== */

void patternSingle(uint32_t color) {
  uint8_t *c = (uint8_t*) &color;
  #ifdef SERIAL_ENABLE_DEBUG
    Serial.write("set color ");
    Serial.print(c[0], DEC);
    Serial.write(" ");
    Serial.print(c[1], DEC);
    Serial.write(" ");
    Serial.print(c[2], DEC);
    Serial.write(" ");
    Serial.print(c[3], DEC);
    Serial.write("\n");
  #endif
  colorBuffer[0].r = c[3];
  colorBuffer[0].g = c[2];
  colorBuffer[0].b = c[1];
  memcpy(colorBuffer+1, colorBuffer, sizeof(colorBuffer) - sizeof(pixel_t));
}

void applyInstant() {
  memcpy(pixels.getPixels(), colorBuffer, sizeof(colorBuffer));
  needShow = true;
}

bool fadeStep() {
  uint8_t *current = pixels.getPixels();
  uint8_t *target = (uint8_t*) colorBuffer;
  bool done = true;
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

  needFade = ! done;
  needShow = ! done;
  return done;
}

/* ===================================================
 *  Protocol methods
 * =================================================== */

bool protoStep() {
  while(protoReadByte()) {
    if (protoVerifyHeader()) {
      protoCopyHeader();
      newCommand = true;
     return true;
    }
  }
  return false;
}

bool protoReadByte() {
  int b = Serial.read();
  if (b >= 0) {
    lastByte = (byte) (b & 0xFF);
    protocolRingBuffer[protocolBufferOffset] = lastByte;
    #ifdef SERIAL_ENABLE_DEBUG
    Serial.write("read byte ");
    Serial.print(protocolBufferOffset, DEC);
    Serial.write(": ");
    Serial.print(lastByte, HEX);
    Serial.write("\n");
    #endif
    protocolBufferOffset++;
    if (protocolBufferOffset >= sizeof(protocolRingBuffer)) {
      protocolBufferOffset = 0;
    }
    return true;
  }
  return false;
}

bool protoVerifyHeader() {
  byte checksum = 0;
  short index = protocolBufferOffset;
  byte b;
  byte check;

  for (int i = 0; i < sizeof(protocolRingBuffer) - 1 /* without checksum */; i++) {
    b = protocolRingBuffer[index];

    /* verify protocol marker */
    if (i < 4) {
      switch (i) {
        case 0:
          check = PROTO_MARKER_0;
          break;
        case 1:
          check = PROTO_MARKER_1;
          break;
        case 2:
          check = PROTO_MARKER_2;
          break;
        case 3:
          check = PROTO_MARKER_3;
          break;
      }
      if (b != check) {
        #ifdef SERIAL_ENABLE_DEBUG
          Serial.write("verify marker ");
          Serial.print(i, DEC);
          Serial.write(" failed\n");
        #endif
        return false;
      }
    } else {
      checksum ^= b;
    }
    
    index++;
    if (index >= sizeof(protocolRingBuffer)) {
      index = 0;
    }
  }

  #ifdef SERIAL_ENABLE_DEBUG
    if (checksum == lastByte) {
      Serial.write("checksum ok\n");
    } else {
      Serial.write("checksum fail\n");
    }
  #endif

  return (checksum == lastByte);
}

void protoCopyHeader() {
  currentHeader.startMarker[0] = protocolRingBuffer[protocolBufferOffset];
  currentHeader.startMarker[1] = protocolRingBuffer[(protocolBufferOffset + 1) % sizeof(protocolRingBuffer)];
  currentHeader.startMarker[2] = protocolRingBuffer[(protocolBufferOffset + 2) % sizeof(protocolRingBuffer)];
  currentHeader.startMarker[3] = protocolRingBuffer[(protocolBufferOffset + 3) % sizeof(protocolRingBuffer)];
  currentHeader.memberAddress = protocolRingBuffer[(protocolBufferOffset + 4) % sizeof(protocolRingBuffer)];
  currentHeader.command = protocolRingBuffer[(protocolBufferOffset + 5) % sizeof(protocolRingBuffer)];
  currentHeader.flags = protocolRingBuffer[(protocolBufferOffset + 6) % sizeof(protocolRingBuffer)];
  ((uint8_t*) &currentHeader.payload)[0] = protocolRingBuffer[(protocolBufferOffset + 7) % sizeof(protocolRingBuffer)];
  ((uint8_t*) &currentHeader.payload)[1] = protocolRingBuffer[(protocolBufferOffset + 8) % sizeof(protocolRingBuffer)];
  ((uint8_t*) &currentHeader.payload)[2] = protocolRingBuffer[(protocolBufferOffset + 9) % sizeof(protocolRingBuffer)];
  ((uint8_t*) &currentHeader.payload)[3] = protocolRingBuffer[(protocolBufferOffset + 10) % sizeof(protocolRingBuffer)];
  currentHeader.checksum = protocolRingBuffer[(protocolBufferOffset + 11) % sizeof(protocolRingBuffer)];
  
  #ifdef SERIAL_ENABLE_DEBUG
  Serial.write("Header: ");
  byte* head = (byte*) &currentHeader;
  for (int i = 0; i < sizeof(protocol_header_t); i++) {
    Serial.print(head[i], HEX);
  }
  Serial.write("\n");
  #endif
}

/* ===================================================
 *  Main methods
 * =================================================== */

void setup() {
  protocolBufferOffset = 0;
  newCommand = false;
  needFade = false;
  needShow = false;
  longPayloadLength = 0;
  longPayloadIndex = 0;
  memset(protocolRingBuffer, 0, sizeof(protocolRingBuffer));

  pixels.begin();
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.write("Ready\n");

  patternSingle(BLACK);
  applyInstant();
}

void loop() {
  protoStep();

  if (newCommand) {
    newCommand = false;
    #ifdef SERIAL_ENABLE_DEBUG
      Serial.write("EXECUTE COMMAND \n");
    #endif
    
    if (currentHeader.memberAddress != MEMBER_ADDRESS) {
      return;
    }

    if (currentHeader.command & PROTO_CMD_PATTERN_SIMPLE) {
      patternSingle(currentHeader.payload);

      if (protoStep()) return;

      if (currentHeader.command & PROTO_MOD_FADE) {
        needFade = true;
      } else {
        needFade = false;
        applyInstant();
      }
    }

    /* TODO: only PROTO_CMD_PATTERN_SIMPLE is supported */

    /* TODO: flags are ignored. 
     * PROTO_FLAG_REPEAT is assumed */

     
     if (protoStep()) return;
  }

  if (needFade) {
    fadeStep();
    if (protoStep()) return;
  }

  if (needShow) {
    pixels.show();
    needShow = false;
  }
}
