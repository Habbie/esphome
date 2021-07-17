#include "esphome.h"

#define A (byte)0x52

class GuitarComponent : public PollingComponent, public TextSensor {
 public:
  class GuitarState {
    // naming based on http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_(Wii)_Guitars
   public:
    byte sx;
    byte sy;
    byte tb;
    byte wb;
    bool bd, bu, bmin, bplus, bo, br, bb, bg, by;

    void decodeResult(byte result[6]) {
      // FIXME: can I replace this with smarter per-bit things directly in the struct def?
      sx = result[0] & 63;
      sy = result[1] & 63;
      tb = result[2] & 31;
      wb = result[3] & 31;
      bd = result[4] & (1<<6);
      bmin = result[4] & (1<<4);
      bplus = result[4] & (1<<2);
      bo = result[5] & (1<<7);
      br = result[5] & (1<<6);
      bb = result[5] & (1<<5);
      bg = result[5] & (1<<4);
      by = result[5] & (1<<3);
      bu = result[5] & (1<<0);
    }

    // shameless rip from wifi_component.cpp
    // LOWER ONE QUARTER BLOCK
    // Unicode: U+2582, UTF-8: E2 96 82
    // LOWER HALF BLOCK
    // Unicode: U+2584, UTF-8: E2 96 84
    // LOWER THREE QUARTERS BLOCK
    // Unicode: U+2586, UTF-8: E2 96 86
    // FULL BLOCK
    // Unicode: U+2588, UTF-8: E2 96 88
    void colorblock(char *buf, bool b) {
      if (!b) strcat(buf, "\xE2\x96\x88");
      else strcat(buf, "\xe2\x96\x82");
    }
    void buttons(char *buf) {
      buf[0]='\0';
      strcat(buf, "\033[0;33m");   colorblock(buf, bo);  // orange
      strcat(buf, "\033[0;34;1m"); colorblock(buf, bb);  // blue
      strcat(buf, "\033[0;33;1m"); colorblock(buf, by);  // yellow
      strcat(buf, "\033[0;31m");   colorblock(buf, br);  // red
      strcat(buf, "\033[0;32m");   colorblock(buf, bg);  // green
      strcat(buf, "\033[0m");
      if (!bu) strcat(buf, "\xE2\x86\x91"); else strcat(buf, " ");
      if (!bd) strcat(buf, "\xE2\x86\x93"); else strcat(buf, " ");
      if (!bmin) strcat(buf, "-"); else strcat(buf, " ");
      if (!bplus) strcat(buf, "+"); else strcat(buf, " ");

    }
  };
  byte id[6];
  byte lastresult[6];

  GuitarComponent() : PollingComponent(100) { ESP_LOGD("guitar", "constructor"); }

  void write_reg(byte addr, byte reg, byte val) {
    ESP_LOGD("guitar", "write_reg"); 
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
  }
    
  void select_read_reg(byte addr, byte reg) {
  //  ESP_LOGD("guitar", "read_reg"); 
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
  }
  void setup() override {
    // Initialize the device here. Usually Wire.begin() will be called in here,
    // though that call is unnecessary if you have an 'i2c:' entry in your config

    // Wire.begin();
    // disable encryption
    write_reg(A, 0xf0, 0x55);
    write_reg(A, 0xfb, 0x00);

    ESP_LOGD("guitar", "disabled encryption"); 
    // read ID
    select_read_reg(A, 0xfa);
    ESP_LOGD("guitar", "asking for ID"); 
    
    Wire.requestFrom(A, size_t(6));
    ESP_LOGD("guitar", "requested 6 bytes");

    for(byte i=0; i<6; i++) {
      if (!Wire.available()) break;
      ESP_LOGD("guitar", "reading byte %d", i);
      id[i] = Wire.read();
    }
    publish_state(hexencode(id, 6));
    bzero(lastresult, 6);
  }
  void update() override {
    byte result[6] = {0,0,0,0,0,0};

//    ESP_LOGD("guitar", "asking for data"); 

    Wire.requestFrom(A, (size_t)6);
 //   ESP_LOGD("guitar", "requested 6 bytes, got %d available", Wire.available());
    for(byte i=0; i<6; i++) {
      if (!Wire.available()) { ESP_LOGD("guitar", "aborting read after %d bytes", i); break; }
  //    ESP_LOGD("guitar", "reading byte %d", i);
      result[i] = Wire.read();
    }
    
    select_read_reg(A, 0x00);

    if (memcmp(lastresult, result, 6) == 0) {
      // nothing changed, not publishing state
      return;
    }

    memcpy(lastresult, result, 6);

    // publish_state(hexencode(result, 6));
    
    GuitarState gs;

    gs.decodeResult(result);
    char buttons[100];
    gs.buttons(buttons);
    ESP_LOGD("guitar", "sx=%2d sy=%2d tb=%2d wb=%2d %s", gs.sx, gs.sy, gs.tb, gs.wb, buttons);
  }
};
