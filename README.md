# ElectraWifi ESPHome external component

This repo converts the original MQTT/Homie-based ElectraWifi project into an **ESPHome external climate component**.

## What it does

- exposes a real `climate` entity in Home Assistant
- sends the same custom Electra IR payload logic from the original project
- supports:
  - modes: `off`, `cool`, `heat`, `heat_cool` (maps to Electra auto), `dry`, `fan_only`
  - fan: `low`, `medium`, `high`, `auto`
  - swing: `off`, `vertical`, `horizontal`, `both`
  - preset: `ifeel`
- optional power feedback pin, like the original repo
- periodic iFeel resend, like the original repo

## Current limits

- IR receive-based state sync is **not implemented yet** in this port.
- transmission currently uses the original raw timing logic directly on the configured IR pin.

## Repo layout

```text
components/
  electrawifi/
    __init__.py
    climate.py
    electrawifi.h
    electrawifi.cpp
    IRelectra.h
    IRelectra.cpp
```

## Example ESPHome YAML

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/YOUR_USER/YOUR_REPO
    components: [electrawifi]

climate:
  - platform: electrawifi
    name: "Living Room AC"
    ir_pin: GPIO4
    power_pin:
      number: GPIO6
      mode:
        input: true
        pullup: true
    # optional
    ir_receive_pin:
      number: GPIO5
      inverted: true
      mode:
        input: true
    support_receive: false
    ifeel_resend_interval: 120s
    power_debounce: 2s
```

## Notes for your hardware

From your last message:
- IR transmit: `GPIO4`
- IR receive dump: `GPIO5` (inverted)

If `GPIO5` is really your IR receiver, do **not** also use it as the original power-sense pin.
Pick a separate power feedback GPIO if you want actual-state tracking.
