#include "IRelectra.h"

#include <cmath>
#include "esphome/core/hal.h"

namespace esphome {
namespace electrawifi {

static const uint16_t UNIT = 1000;
static const uint8_t NUM_BITS = 34;

IRelectra::IRelectra(uint8_t output_pin) : output_pin_(output_pin) {
  power_setting = POWER_KEEP;
  power_real = false;
  mode = MODE_COOL;
  fan = FAN_LOW;
  temperature = 26;
  swing_h = SWING_H_OFF;
  swing = SWING_OFF;
  sleep = SLEEP_OFF;
  ifeel_temperature = 0;
  ifeel = IFEEL_OFF;
}

void IRelectra::addBit(unsigned int *p, int *i, char b) {
  if (((*i) & 1) == 1) {
    if ((b & 1) == 1) {
      *(p + *i) += UNIT;
      (*i)++;
      *(p + *i) = UNIT;
    }
    if ((b & 1) == 0) {
      (*i)++;
      *(p + *i) = UNIT;
      (*i)++;
      *(p + *i) = UNIT;
    }
  } else {
    if ((b & 1) == 1) {
      (*i)++;
      *(p + *i) = UNIT;
      (*i)++;
      *(p + *i) = UNIT;
    }
    if ((b & 1) == 0) {
      *(p + *i) += UNIT;
      (*i)++;
      *(p + *i) = UNIT;
    }
  }
}

void IRelectra::SendElectra(bool notify) {
  uint data[200];
  int i = 0;
  const uint64_t code = EncodeElectra(notify);

  for (int k = 0; k < 3; k++) {
    data[i] = 3 * UNIT;
    i++;
    data[i] = 3 * UNIT;
    for (int j = NUM_BITS - 1; j >= 0; j--) {
      addBit(data, &i, (code >> j) & 1);
    }
    i++;
  }
  data[i] = 4 * UNIT;

  SendRaw(data, i + 1);
}

void IRelectra::SendRaw(unsigned int *data, unsigned int size) {
  if (!this->pin_writer_)
    return;
  for (unsigned int i = 0; i < size; ++i) {
    this->pin_writer_((i % 2) == 0);
    delayMicroseconds(data[i]);
  }
  this->pin_writer_(false);
}

uint64_t IRelectra::EncodeElectra(bool notify) {
  uint64_t code = 0;
  uint32_t send_temp;
  power_t power;

  power = (power_setting == power_real) ? POWER_KEEP : POWER_TOGGLE;
  send_temp = notify ? (ifeel_temperature - 5) : (temperature - 15);

  code |= (((uint64_t) power & 1) << 33);
  code |= (((uint64_t) mode & 7) << 30);
  code |= (((uint64_t) fan & 3) << 28);
  code |= (((uint64_t) notify & 1) << 27);
  code |= (((uint64_t) swing_h & 1) << 26);
  code |= (((uint64_t) swing & 1) << 25);
  code |= (((uint64_t) ifeel & 1) << 24);
  code |= (((uint64_t) send_temp & 31) << 19);
  code |= (((uint64_t) sleep & 1) << 18);
  code |= 2;

  return code;
}

void IRelectra::UpdateFromIR(uint64_t code) {
  uint32_t send_temp;
  power_t power;
  bool notify;

  power = (power_t) ((code >> 33) & 1);
  mode = (ac_mode_t) ((code >> 30) & 7);
  fan = (fan_t) ((code >> 28) & 3);
  notify = (bool) ((code >> 27) & 1);
  swing_h = (swing_h_t) ((code >> 26) & 1);
  swing = (swing_t) ((code >> 25) & 1);
  ifeel = (ifeel_t) ((code >> 24) & 1);
  send_temp = (uint32_t) ((code >> 19) & 31);
  sleep = (sleep_t) ((code >> 18) & 1);

  if (power == POWER_TOGGLE) {
    power_setting = !power_real;
  }

  if (notify) {
    ifeel_temperature = send_temp + 5;
  } else {
    temperature = send_temp + 15;
  }
}

}  // namespace electrawifi
}  // namespace esphome
