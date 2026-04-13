#include "electrawifi.h"

#include "esphome/core/log.h"

namespace esphome {
namespace electrawifi {

static const char *const TAG = "electrawifi.climate";

static const char *const PRESET_NONE = "none";
static const char *const PRESET_IFEEL = "ifeel";

void ElectraWifiClimate::setup() {
  if (this->ir_pin_ != nullptr) {
    this->ir_pin_->setup();
    this->ir_pin_->digital_write(false);
  }
  if (this->power_pin_ != nullptr) {
    this->power_pin_->setup();
  }
  if (this->ir_receive_pin_ != nullptr) {
    this->ir_receive_pin_->setup();
  }

  this->ac_ = IRelectra(0);
  this->ac_.set_pin_writer([this](bool level) {
    if (this->ir_pin_ != nullptr) this->ir_pin_->digital_write(level);
  });

  this->ac_.power_setting = false;
  this->ac_.power_real = false;
  this->ac_.mode = MODE_COOL;
  this->ac_.fan = FAN_AUTO;
  this->ac_.temperature = this->default_temperature_;
  this->ac_.swing = SWING_OFF;
  this->ac_.swing_h = SWING_H_OFF;
  this->ac_.ifeel = IFEEL_OFF;
  this->ac_.ifeel_temperature = this->default_ifeel_temperature_;

  if (this->power_pin_ != nullptr) {
    const bool power_state = !this->power_pin_->digital_read();
    this->ac_.power_real = power_state;
    this->ac_.power_setting = power_state;
  }

  this->sync_from_runtime_state_();
  this->publish_state();
}

void ElectraWifiClimate::update() {
  this->reconcile_power_pin_();

  const uint32_t now = millis();
  if (this->ac_.ifeel == IFEEL_ON && now - this->last_ifeel_send_ >= this->ifeel_resend_interval_ms_) {
    ESP_LOGD(TAG, "Sending periodic iFeel update: %u", this->ac_.ifeel_temperature);
    this->send_state_command_(true);
    this->last_ifeel_send_ = now;
  }
}

void ElectraWifiClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "ElectraWifi Climate");
  LOG_CLIMATE("  ", "ElectraWifi Climate", this);
  LOG_PIN("  IR pin: ", this->ir_pin_);
  LOG_PIN("  Power pin: ", this->power_pin_);
  LOG_PIN("  IR receive pin: ", this->ir_receive_pin_);
  ESP_LOGCONFIG(TAG, "  Receive support requested: %s", YESNO(this->support_receive_));
  if (this->support_receive_ && this->ir_receive_pin_ != nullptr) {
    ESP_LOGW(TAG, "IR receive state syncing is not implemented yet; receiver pin is only reserved for future use.");
  }
  ESP_LOGCONFIG(TAG, "  iFeel resend interval: %u ms", this->ifeel_resend_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Power debounce: %u ms", this->power_debounce_ms_);
}

climate::ClimateTraits ElectraWifiClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(false);
  traits.set_supports_two_point_target_temperature(false);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_HEAT_COOL,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
      climate::CLIMATE_FAN_AUTO,
  });
  traits.set_supported_swing_modes({
      climate::CLIMATE_SWING_OFF,
      climate::CLIMATE_SWING_VERTICAL,
      climate::CLIMATE_SWING_HORIZONTAL,
      climate::CLIMATE_SWING_BOTH,
  });
  traits.set_supported_presets({
    climate::CLIMATE_PRESET_NONE,
    climate::CLIMATE_PRESET_ECO
});
  traits.set_visual_min_temperature(15);
  traits.set_visual_max_temperature(30);
  traits.set_visual_temperature_step(1.0f);
  return traits;
}

void ElectraWifiClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    switch (*call.get_mode()) {
      case climate::CLIMATE_MODE_OFF:
        this->ac_.power_setting = false;
        break;
      case climate::CLIMATE_MODE_COOL:
        this->ac_.power_setting = true;
        this->ac_.mode = MODE_COOL;
        break;
      case climate::CLIMATE_MODE_HEAT:
        this->ac_.power_setting = true;
        this->ac_.mode = MODE_HEAT;
        break;
      case climate::CLIMATE_MODE_HEAT_COOL:
        this->ac_.power_setting = true;
        this->ac_.mode = MODE_AUTO;
        break;
      case climate::CLIMATE_MODE_DRY:
        this->ac_.power_setting = true;
        this->ac_.mode = MODE_DRY;
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        this->ac_.power_setting = true;
        this->ac_.mode = MODE_FAN;
        break;
      default:
        break;
    }
  }

  if (call.get_target_temperature().has_value()) {
    auto target = static_cast<int>(lroundf(*call.get_target_temperature()));
    if (target < 15)
      target = 15;
    if (target > 30)
      target = 30;
    this->ac_.temperature = static_cast<uint8_t>(target);
  }

  if (call.get_fan_mode().has_value()) {
    this->set_fan_mode_(*call.get_fan_mode());
  }

  if (call.get_swing_mode().has_value()) {
    this->set_swing_flags_(*call.get_swing_mode());
  }

  if (call.get_preset().has_value()) {
    this->set_ifeel_from_preset_(*call.get_preset());
  }

  this->send_state_command_(false);

  if (this->power_pin_ == nullptr) {
    this->ac_.power_real = this->ac_.power_setting;
  }

  this->sync_from_runtime_state_();
  this->publish_state();
}

void ElectraWifiClimate::reconcile_power_pin_() {
  if (this->power_pin_ == nullptr)
    return;

  const bool sampled = !this->power_pin_->digital_read();
  const uint32_t now = millis();

  if (sampled != this->ac_.power_real) {
    if (!this->pending_power_sample_valid_ || sampled != this->pending_power_sample_) {
      this->pending_power_sample_ = sampled;
      this->pending_power_sample_valid_ = true;
      this->power_change_time_ = now;
      return;
    }
    if (now - this->power_change_time_ >= this->power_debounce_ms_) {
      this->ac_.power_real = sampled;
      this->ac_.power_setting = sampled;
      this->pending_power_sample_valid_ = false;
      this->sync_from_runtime_state_();
      this->publish_state();
    }
  } else {
    this->pending_power_sample_valid_ = false;
  }
}

void ElectraWifiClimate::sync_from_runtime_state_() {
  this->mode = this->current_mode_();
  this->target_temperature = this->ac_.temperature;
  this->fan_mode = this->current_fan_mode_();
  this->swing_mode = this->swing_mode_from_flags_();
  this->preset = this->preset_from_ifeel_();
}

climate::ClimatePreset ElectraWifiClimate::preset_from_ifeel_() const {
  return this->ac_.ifeel == IFEEL_ON
  ? climate::CLIMATE_PRESET_ECO
  : climate::CLIMATE_PRESET_NONE;

void ElectraWifiClimate::set_ifeel_from_preset_(climate::ClimatePreset preset) {
  if (preset == climate::CLIMATE_PRESET_ECO) {
    this->ac_.ifeel = IFEEL_ON;
  } else {
    this->ac_.ifeel = IFEEL_OFF;
  }
}

climate::ClimateSwingMode ElectraWifiClimate::swing_mode_from_flags_() const {
  if (this->ac_.swing == SWING_ON && this->ac_.swing_h == SWING_H_ON)
    return climate::CLIMATE_SWING_BOTH;
  if (this->ac_.swing == SWING_ON)
    return climate::CLIMATE_SWING_VERTICAL;
  if (this->ac_.swing_h == SWING_H_ON)
    return climate::CLIMATE_SWING_HORIZONTAL;
  return climate::CLIMATE_SWING_OFF;
}

void ElectraWifiClimate::set_swing_flags_(climate::ClimateSwingMode mode) {
  switch (mode) {
    case climate::CLIMATE_SWING_BOTH:
      this->ac_.swing = SWING_ON;
      this->ac_.swing_h = SWING_H_ON;
      break;
    case climate::CLIMATE_SWING_VERTICAL:
      this->ac_.swing = SWING_ON;
      this->ac_.swing_h = SWING_H_OFF;
      break;
    case climate::CLIMATE_SWING_HORIZONTAL:
      this->ac_.swing = SWING_OFF;
      this->ac_.swing_h = SWING_H_ON;
      break;
    default:
      this->ac_.swing = SWING_OFF;
      this->ac_.swing_h = SWING_H_OFF;
      break;
  }
}

void ElectraWifiClimate::set_fan_mode_(climate::ClimateFanMode mode) {
  switch (mode) {
    case climate::CLIMATE_FAN_LOW:
      this->ac_.fan = FAN_LOW;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      this->ac_.fan = FAN_MED;
      break;
    case climate::CLIMATE_FAN_HIGH:
      this->ac_.fan = FAN_HIGH;
      break;
    default:
      this->ac_.fan = FAN_AUTO;
      break;
  }
}

climate::ClimateFanMode ElectraWifiClimate::current_fan_mode_() const {
  switch (this->ac_.fan) {
    case FAN_LOW:
      return climate::CLIMATE_FAN_LOW;
    case FAN_MED:
      return climate::CLIMATE_FAN_MEDIUM;
    case FAN_HIGH:
      return climate::CLIMATE_FAN_HIGH;
    default:
      return climate::CLIMATE_FAN_AUTO;
  }
}

climate::ClimateMode ElectraWifiClimate::current_mode_() const {
  if (!this->ac_.power_real)
    return climate::CLIMATE_MODE_OFF;

  switch (this->ac_.mode) {
    case MODE_COOL:
      return climate::CLIMATE_MODE_COOL;
    case MODE_HEAT:
      return climate::CLIMATE_MODE_HEAT;
    case MODE_AUTO:
      return climate::CLIMATE_MODE_HEAT_COOL;
    case MODE_DRY:
      return climate::CLIMATE_MODE_DRY;
    case MODE_FAN:
      return climate::CLIMATE_MODE_FAN_ONLY;
    default:
      return climate::CLIMATE_MODE_COOL;
  }
}

void ElectraWifiClimate::send_state_command_(bool notify) {
  this->ac_.SendElectra(notify);
}

}  // namespace electrawifi
}  // namespace esphome
