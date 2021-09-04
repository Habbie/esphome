#include "pm1006.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pm1006 {

static const char *const TAG = "pm1006";

static const uint8_t PM1006_REQUEST = 0x11;
static const uint8_t PM1006_RESPONSE = 0x16;

static const uint8_t PM1006_CMD_PM25 = 0x0B;
static const uint8_t PM1006_CMD_DATA_PM25[] = {0x01}; // FIXME: 0x07 here leads to a crash! because the response message has PM1006_CMD_PM25 but the data section is empty, and we try to .at(5) which is one byte beyond the end of an empty message and the exception from .at bubbles all the way up to a full ESP crash

static const uint8_t PM1006_CMD_VERSION = 0x1E;
static const uint8_t PM1006_CMD_SERIAL_NUMBER = 0x1F;

#define WRAPDATA(data) data, sizeof(data)

void PM1006Component::setup() {
  auto req = make_request_(PM1006_CMD_VERSION);
  this->write_array(req);
  req = make_request_(PM1006_CMD_SERIAL_NUMBER);
  this->write_array(req);
}

void PM1006Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PM1006:");
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  ESP_LOGCONFIG(TAG, "  Reversed mode: %s", ONOFF(this->reversed_));
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(9600); // FIXME: can go, part of config validation now
}

std::vector<uint8_t> PM1006Component::make_request_(const uint8_t cmd, const uint8_t* data, const uint8_t len)
{
  return make_frame_(PM1006_REQUEST, cmd, data, len);
}

std::vector<uint8_t> PM1006Component::make_frame_(const uint8_t header, const uint8_t cmd, const uint8_t* data, const uint8_t len)
{
  std::vector<uint8_t> req;
  req.reserve(1 + 1 + len + 1); // PM1006_REQUEST/RESPONSE + LEN + cmd + CHECKSUM
  req.push_back(header);
  req.push_back(len+1); // +1 for the cmd
  req.push_back(cmd);
  if (data) {
    for (uint8_t i = 0; i < len; i++)
      req.push_back(data[i]);
  }

  uint8_t sum = pm1006_checksum_(req);
  req.push_back(256 - sum);

  return req;
}

void PM1006Component::update() {
  std::vector<uint8_t> req = make_request_(PM1006_CMD_PM25, WRAPDATA(PM1006_CMD_DATA_PM25));
  ESP_LOGV(TAG, "sending measurement request");
  this->write_array(req);
}

enum class RXState { Begin, GotHeader, GotLen, GotBody, GotChecksum, Invalid };

void PM1006Component::loop() {
  RXState state = RXState::Begin;
  data_.resize(0);
  data_.reserve(20);
  uint8_t len = 0, checksum = 0;
  uint8_t expected_header = reversed_ ? PM1006_REQUEST : PM1006_RESPONSE;
  while (state != RXState::GotChecksum && this->available() != 0) {
    uint8_t byte;
    this->read_byte(&byte);
    switch (state) {
      case RXState::Begin:
        if (byte == expected_header) {
          data_.push_back(byte);
          state = RXState::GotHeader;
        }
        else {
          ESP_LOGW(TAG, "PM1006: got wrong header (%02x), expected %02x", byte, expected_header);
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

void PM1006Component::parse_data_() {
  uint8_t msgtype = data_.at(2);

  switch (msgtype) {
    case PM1006_CMD_PM25: {
      if (reversed_) {
        std::vector<uint8_t> data = {0, 0, 0, 80 /* DF4 */, 0, 0, 0, 0 /* DF8 */, 0, 0, 0, 0 /* DF12 */, 0, 0, 0, 0 /* DF16 */};
        auto response = make_frame_(PM1006_RESPONSE, PM1006_CMD_PM25, data.data(), data.size());
        this->write_array(response);
        break;
      }
      const int pm_2_5_concentration = this->get_16_bit_uint_(5);

      ESP_LOGD(TAG, "Got PM2.5 Concentration: %d µg/m³", pm_2_5_concentration);

      if (this->pm_2_5_sensor_ != nullptr) {
        this->pm_2_5_sensor_->publish_state(pm_2_5_concentration);
      }
      break;
    }
    case PM1006_CMD_VERSION: {
      std::string version;
      for (uint8_t i = 3; i < data_.size() - 1; i++)
        version.push_back(data_.at(i));
      ESP_LOGD(TAG, "Got version number: %s", version.c_str());
      break;
    }
    default:
      ESP_LOGD(TAG, "Got unknown message type from sensor: ");
      ESP_LOGD(TAG, "  Length: %d", data_.at(1));
      ESP_LOGD(TAG, "  Type: %02x", data_.at(2));
      ESP_LOGD(TAG, "  Full message: %s", hexencode(data_).c_str());
      break;
  }
}

uint16_t PM1006Component::get_16_bit_uint_(uint8_t start_index) const {
  return encode_uint16(this->data_.at(start_index), this->data_.at(start_index + 1));
}

}  // namespace pm1006
}  // namespace esphome
