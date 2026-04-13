#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "IRelectra.h"

namespace esphome {
namespace electrawifi {

class ElectraWifiClimate : public climate::Climate, public PollingComponent {
 public:
  void set_ir_pin(GPIOPin *pin) { this->ir_pin_ = pin; }
  void set_power_pin(GPIOPin *pin) { this->power_pin_ = pin; }
  void set_ir_receive_pin(GPIOPin *pin) { this->ir_receive_pin_ = pin; }
  void set_support_receive(bool support_receive) { this->support_receive_ = support_receive; }
  void set_ifeel_resend_interval_ms(uint32_t ms) { this->ifeel_resend_interval_ms_ = ms; }
  void set_power_debounce_ms(uint32_t ms) { this->power_debounce_ms_ = ms; }
  void set_default_temperature(uint8_t temp) { this->default_temperature_ = temp; }
  void set_default_ifeel_temperature(uint8_t temp) { this->default_ifeel_temperature_ = temp; }

  void setup() override;
  void update() override;
  void dump_config() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

 protected:
  void sync_from_runtime_state_();
  void sync_to_runtime_state_();
  void send_state_command_(bool notify);
  void reconcile_power_pin_();
  climate::ClimatePreset preset_from_ifeel_() const;
  void set_ifeel_from_preset_(const std::string &preset);
  climate::ClimateSwingMode swing_mode_from_flags_() const;
  void set_swing_flags_(climate::ClimateSwingMode mode);
  void set_fan_mode_(climate::ClimateFanMode mode);
  climate::ClimateFanMode current_fan_mode_() const;
  climate::ClimateMode current_mode_() const;

  GPIOPin *ir_pin_{nullptr};
  GPIOPin *power_pin_{nullptr};
  GPIOPin *ir_receive_pin_{nullptr};
  bool support_receive_{false};
  uint32_t ifeel_resend_interval_ms_{120000};
  uint32_t power_debounce_ms_{2000};
  uint8_t default_temperature_{26};
  uint8_t default_ifeel_temperature_{25};
  uint32_t power_change_time_{0};
  uint32_t last_ifeel_send_{0};
  bool pending_power_sample_valid_{false};
  bool pending_power_sample_{false};

  IRelectra ac_{0};
};

}  // namespace electrawifi
}  // namespace esphome
