#define BT_width 8
#define BT_height 10

#define BAT_width 20
#define BAT_height 9

#define WIFI_width 14
#define WIFI_height 8

const unsigned char BT_bits[] PROGMEM = {
  0x18, 0x28, 0x4A, 0x2C, 0x18, 0x2C, 0x4A, 0x28, 0x18, 0x00,
  };

const unsigned char BAT_bits[] PROGMEM = {
  0xFC, 0xFF, 0x0F, 0x04, 0x00, 0x08, 0xF7, 0xDE, 0x0B, 0xF1, 0xDE, 0x0B, 
  0xF1, 0xDE, 0x0B, 0xF1, 0xDE, 0x0B, 0xF7, 0xDE, 0x0B, 0x04, 0x00, 0x08, 
  0xFC, 0xFF, 0x0F,
  };

const unsigned char WIFI_bits[] PROGMEM = {
  0xF0, 0x03, 0x04, 0x08, 0xF2, 0x13, 0x09, 0x24, 0xE4, 0x09, 0x10, 0x02, 
  0xC0, 0x00, 0xC0, 0x00,
  };

const unsigned char activeSymbol[] PROGMEM = {
    B00000000,
    B00000000,
    B00011000,
    B00100100,
    B01000010,
    B01000010,
    B00100100,
    B00011000
};

const unsigned char inactiveSymbol[] PROGMEM = {
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00011000,
    B00011000,
    B00000000,
    B00000000
};

//Added by Sloeber 
#pragma once


//Added by Sloeber 
#pragma once
