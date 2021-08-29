#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace pm1006 {

class PM1006Component : public PollingComponent, public uart::UARTDevice {
 public:
  PM1006Component() = default;

  void set_pm_2_5_sensor(sensor::Sensor *pm_2_5_sensor) { this->pm_2_5_sensor_ = pm_2_5_sensor; }
  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;

  float get_setup_priority() const override;

 protected:
std::vector<uint8_t> make_request_(const uint8_t cmd, const uint8_t* data=nullptr, const uint8_t len=0);
  void parse_data_() const;
  uint16_t get_16_bit_uint_(uint8_t start_index) const;
  uint8_t pm1006_checksum_(const std::vector<uint8_t> data) const;

  sensor::Sensor *pm_2_5_sensor_{nullptr};

  std::vector<uint8_t> data_;
};

}  // namespace pm1006
}  // namespace esphome
