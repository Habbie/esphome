#include "pm1006.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pm1006 {

static const char *const TAG = "pm1006";

static const uint8_t PM1006_REQUEST = 0x11;
static const uint8_t PM1006_RESPONSE = 0x16;

static const uint8_t PM1006_REQ_PM25[] = {0x0B, 0x01};

#define WRAPCMD(cmd) cmd, sizeof(cmd)
void PM1006Component::setup() {
  // because this implementation is currently rx-only, there is nothing to setup
}

void PM1006Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PM1006:");
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  this->check_uart_settings(9600);
}

std::vector<uint8_t> PM1006Component::make_request_(const uint8_t* cmd, const uint8_t len)
{
  std::vector<uint8_t> req;
  req.reserve(1 + 1 + len + 1); // PM1006_REQUEST + LEN + cmd + CHECKSUM
  req.push_back(PM1006_REQUEST);
  req.push_back(len);
  for (uint8_t i = 0; i < len; i++)
    req.push_back(cmd[i]);

  uint8_t sum = pm1006_checksum_(req);
  req.push_back(256 - sum);

  return req;
}

void PM1006Component::update() {
  std::vector<uint8_t> req = make_request_(WRAPCMD(PM1006_REQ_PM25));
  ESP_LOGV(TAG, "sending measurement request");
  this->write_array(req);
}

enum class RXState { Begin, GotHeader, GotLen, GotBody, GotChecksum, Invalid };

void PM1006Component::loop() {
  RXState state = RXState::Begin;
  data_.resize(0);
  data_.reserve(20);
  uint8_t len = 0, checksum = 0;
  while (state != RXState::GotChecksum && this->available() != 0) {
    uint8_t byte;
    this->read_byte(&byte);
    switch (state) {
      case RXState::Begin:
        if (byte == PM1006_RESPONSE) {
          data_.push_back(byte);
          state = RXState::GotHeader;
        }
        else {
          state = RXState::Invalid;
        }
        break;
      case RXState::GotHeader:
        data_.push_back(byte);
        len = byte;
        state = RXState::GotLen;
        break;
      case RXState::GotLen:
        data_.push_back(byte);
        if (data_.size() == len + 2) {
          state = RXState::GotBody;
        }
        break;
      case RXState::GotBody:
        data_.push_back(byte);
        checksum = pm1006_checksum_(data_);
        if (checksum != 0) {
          ESP_LOGW(TAG, "PM1006 checksum is wrong: %02x, expected zero", checksum);
          state = RXState::Invalid;
        }
        else {
          state = RXState::GotChecksum;
        }
        break;
      case RXState::GotChecksum:
        // cannot actually get here
        break;
      case RXState::Invalid:
        // let's just finish this read and hope the next one is better
        break;
    }
  }

  if (state == RXState::GotChecksum) {
    parse_data_();
  }
}

float PM1006Component::get_setup_priority() const { return setup_priority::DATA; }

uint8_t PM1006Component::pm1006_checksum_(const std::vector<uint8_t> data) const {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < data.size(); i++) {
    sum += data[i];
  }
  return sum;
}

void PM1006Component::parse_data_() const {
  const int pm_2_5_concentration = this->get_16_bit_uint_(5);

  ESP_LOGD(TAG, "Got PM2.5 Concentration: %d µg/m³", pm_2_5_concentration);

  if (this->pm_2_5_sensor_ != nullptr) {
    this->pm_2_5_sensor_->publish_state(pm_2_5_concentration);
  }
}

uint16_t PM1006Component::get_16_bit_uint_(uint8_t start_index) const {
  return encode_uint16(this->data_[start_index], this->data_[start_index + 1]);
}

}  // namespace pm1006
}  // namespace esphome
