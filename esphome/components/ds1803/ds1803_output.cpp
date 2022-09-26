#include "ds1803_output.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ds1803 {

static const char *const TAG = "ds1803";

static const uint8_t DS1803_WRITE_CHANNEL_0 = 0xA9;
static const uint8_t DS1803_WRITE_CHANNEL_1 = 0xAA;
// FIXME: add third channel (channel 2) that writes to both pots at the same time?

void DS1803Output::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DS1803OutputComponent...");

}

void DS1803Output::dump_config() {
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Setting up DS1803 failed!");
  } else
    ESP_LOGCONFIG(TAG, "DS1803 initialised");
}

void DS1803Output::register_channel(DS1803Channel *channel) {
  auto c = channel->channel_;
  channel->set_parent(this);
  ESP_LOGV(TAG, "Registered channel: %01u", channel->channel_);
}

void DS1803Output::set_channel_value_(uint8_t channel, uint8_t value) {
  ESP_LOGV(TAG, "Channel %u: setting %u", channel, value);
  if (!this->write_byte(channel ? DS1803_WRITE_CHANNEL_1 : DS1803_WRITE_CHANNEL_0, value)) {
    this->status_set_warning();
  }
  this->status_clear_warning();
}

void DS1803Channel::write_state(float state) {
  ESP_LOGV(TAG, "channel %01u value %f", this->channel_, state);
  uint8_t input = state * 255;
  ESP_LOGV(TAG, "channel %01u decimal %u", this->channel_, input);
  this->parent_->set_channel_value_(this->channel_, input);
}

}  // namespace ds1803
}  // namespace esphome
