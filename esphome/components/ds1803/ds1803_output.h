#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace ds1803 {

class DS1803Output;

class DS1803Channel : public output::FloatOutput, public Parented<DS1803Output> {
 public:
  void set_channel(uint8_t channel) { channel_ = channel; }

 protected:
  friend class DS1803Output;

  void write_state(float state) override;

  uint8_t channel_;
};

/// DS1803 float output component.
class DS1803Output : public Component, public i2c::I2CDevice {
 public:
  DS1803Output() {}

  void register_channel(DS1803Channel *channel);

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  friend DS1803Channel;

  void set_channel_value_(uint8_t channel, uint8_t value);
};

}  // namespace ds1803
}  // namespace esphome
