#pragma once

#include <stdint.h>
#include <functional>

namespace esphome {
namespace electrawifi {

enum power_t {
  POWER_KEEP = 0,
  POWER_TOGGLE = 1
};

enum ac_mode_t {
  MODE_COOL = 0b001,
  MODE_HEAT = 0b010,
  MODE_AUTO = 0b011,
  MODE_DRY = 0b100,
  MODE_FAN = 0b101,
};

enum fan_t {
  FAN_LOW = 0b00,
  FAN_MED = 0b01,
  FAN_HIGH = 0b10,
  FAN_AUTO = 0b11
};

enum swing_t {
  SWING_OFF = 0,
  SWING_ON = 1
};

enum swing_h_t {
  SWING_H_OFF = 0,
  SWING_H_ON = 1
};

enum sleep_t {
  SLEEP_OFF = 0,
  SLEEP_ON = 1
};

enum ifeel_t {
  IFEEL_OFF = 0,
  IFEEL_ON = 1
};

class IRelectra {
 public:
  explicit IRelectra(uint8_t output_pin = 0);

  void set_pin_writer(std::function<void(bool)> writer) { this->pin_writer_ = writer; }
  void SendElectra(bool notify);
  void UpdateFromIR(uint64_t code);

  bool power_setting;
  bool power_real;
  ac_mode_t mode;
  fan_t fan;
  uint8_t temperature;
  swing_t swing;
  swing_h_t swing_h;
  sleep_t sleep;
  uint8_t ifeel_temperature;
  ifeel_t ifeel;

 protected:
  uint8_t output_pin_;
  std::function<void(bool)> pin_writer_;

  uint64_t EncodeElectra(bool notify);
  void addBit(unsigned int *p, int *i, char b);
  void SendRaw(unsigned int *data, unsigned int size);
};

}  // namespace electrawifi
}  // namespace esphome
