import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID
from esphome import pins

AUTO_LOAD = ["climate"]
DEPENDENCIES = ["remote_base"]

CONF_IR_PIN = "ir_pin"
CONF_POWER_PIN = "power_pin"
CONF_IR_RECEIVE_PIN = "ir_receive_pin"
CONF_SUPPORT_RECEIVE = "support_receive"
CONF_IFEEL_RESEND_INTERVAL = "ifeel_resend_interval"
CONF_POWER_DEBOUNCE = "power_debounce"
CONF_DEFAULT_TEMPERATURE = "default_temperature"
CONF_DEFAULT_IFEEL_TEMPERATURE = "default_ifeel_temperature"

ns = cg.esphome_ns.namespace("electrawifi")
ElectraWifiClimate = ns.class_("ElectraWifiClimate", climate.Climate, cg.PollingComponent)

BASE_CLIMATE_SCHEMA = getattr(climate, "CLIMATE_SCHEMA", climate._CLIMATE_SCHEMA)

CONFIG_SCHEMA = BASE_CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ElectraWifiClimate),
        cv.Required(CONF_IR_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_POWER_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_IR_RECEIVE_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_SUPPORT_RECEIVE, default=False): cv.boolean,
        cv.Optional(CONF_IFEEL_RESEND_INTERVAL, default="120s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_POWER_DEBOUNCE, default="2s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_DEFAULT_TEMPERATURE, default=26): cv.int_range(min=15, max=30),
        cv.Optional(CONF_DEFAULT_IFEEL_TEMPERATURE, default=25): cv.int_range(min=5, max=36),
    }
).extend(cv.polling_component_schema("100ms"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    ir_pin = await cg.gpio_pin_expression(config[CONF_IR_PIN])
    cg.add(var.set_ir_pin(ir_pin))

    if CONF_POWER_PIN in config:
        power_pin = await cg.gpio_pin_expression(config[CONF_POWER_PIN])
        cg.add(var.set_power_pin(power_pin))

    if CONF_IR_RECEIVE_PIN in config:
        recv_pin = await cg.gpio_pin_expression(config[CONF_IR_RECEIVE_PIN])
        cg.add(var.set_ir_receive_pin(recv_pin))

    cg.add(var.set_support_receive(config[CONF_SUPPORT_RECEIVE]))
    cg.add(var.set_ifeel_resend_interval_ms(config[CONF_IFEEL_RESEND_INTERVAL].total_milliseconds))
    cg.add(var.set_power_debounce_ms(config[CONF_POWER_DEBOUNCE].total_milliseconds))
    cg.add(var.set_default_temperature(config[CONF_DEFAULT_TEMPERATURE]))
    cg.add(var.set_default_ifeel_temperature(config[CONF_DEFAULT_IFEEL_TEMPERATURE]))
