#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "IRelectra.h"

namespace esphome {
namespace electrawifi {

class ElectraWifiClimate : public climate::Climate, public PollingComponent {
 public:
  // setters
  void set_ir_pin(GPIOPin *pin) { this->ir_pin_ = pin; }
  void set_power_pin(GPIOPin *pin) { this->power_pin_ = pin; }
  void set_ir_receive_pin(GPIOPin *pin) { this->ir_receive_pin_ = pin; }
  void set_support_receive(bool support_receive) { this->support_receive_ = support_receive; }
  void set_ifeel_resend_interval_ms(uint32_t ms) { this->ifeel_resend_interval_ms_ = ms; }
  void set_power_debounce_ms(uint32_t ms) { this->power_debounce_ms_ = ms; }
  void set_default_temperature(uint8_t temp) { this->default_temperature_ = temp; }
  void set_default_ifeel_temperature(uint8_t temp) { this->default_ifeel_temperature_ = temp; }

  // core overrides
  void setup() override;
  void update() override;
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  void dump_config() override;

 protected:
  // 🔧 INTERNAL STATE (THIS WAS MISSING → caused 90% of errors)
  GPIOPin *ir_pin_{nullptr};
  GPIOPin *power_pin_{nullptr};
  GPIOPin *ir_receive_pin_{nullptr};

  bool support_receive_{false};

  uint32_t ifeel_resend_interval_ms_{60000};
  uint32_t power_debounce_ms_{200};

  uint8_t default_temperature_{24};
  uint8_t default_ifeel_temperature_{24};

  uint32_t last_ifeel_send_{0};

  bool pending_power_sample_valid_{false};
  bool pending_power_sample_{false};
  uint32_t power_change_time_{0};

  IRelectra ac_;

  // helpers
  void set_ifeel_from_preset_(climate::ClimatePreset preset);
  climate::ClimatePreset preset_from_ifeel_() const;

  void reconcile_power_pin_();
  void sync_from_runtime_state_();

  void set_swing_flags_(climate::ClimateSwingMode mode);
  climate::ClimateSwingMode swing_mode_from_flags_() const;

  void set_fan_mode_(climate::ClimateFanMode mode);
  climate::ClimateFanMode current_fan_mode_() const;

  climate::ClimateMode current_mode_() const;

  void send_state_command_(bool notify);
};

}  // namespace electrawifi
}  // namespace esphome