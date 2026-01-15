# ZMK Input Processor: Speed Curve

Custom piecewise linear speed curves for ZMK mouse movement. Configure acceleration like a fan curve!

## Quick Start

Add to `config/west.yml`:
```yaml
- name: zmk-input-processor-speed-curve
  url: https://github.com/N3M0-dev/zmk-input-processor-speed-curve
  revision: main
```

Enable in config:
```properties
CONFIG_ZMK_INPUT_PROCESSOR_SPEED_CURVE=y
```

Define your curve:
```devicetree
zip_speed_curve_xy: zip_speed_curve_xy {
    compatible = "zmk,input-processor-speed-curve";
    type = <INPUT_EV_REL>;
    codes = <INPUT_REL_X>, <INPUT_REL_Y>;
    curve-points = <0 50>, <300 200>, <1000 800>;
    trigger-period-ms = <16>;
};
```

📖 **[Full Documentation](README.md)**
