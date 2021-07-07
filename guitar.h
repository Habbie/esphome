#include "esphome.h"

#define A (byte)0x52

class GuitarComponent : public PollingComponent, public TextSensor {
 public:
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

    publish_state(hexencode(result, 6));
  }
};
