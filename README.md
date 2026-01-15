# ZMK Input Processor: Speed Curve

A ZMK input processor that applies custom piecewise linear speed curves to mouse movement, similar to configuring a fan curve. Define acceleration behavior using time-speed coordinate pairs.

## Features

- **Piecewise Linear Interpolation**: Define speed curve using multiple control points
- **Fan Curve Style Configuration**: Intuitive [time_ms, speed_px_per_sec] pairs
- **Automatic Timing**: Tracks elapsed time and resets on direction change or stop
- **Sub-pixel Accuracy**: Optional remainder tracking for smooth low-speed movement
- **Independent Axis Support**: Apply curves to X, Y axes independently or together

## How It Works

The processor:
1. Tracks timing from when movement starts
2. Calculates current speed by interpolating between your defined curve points
3. Converts speed (px/sec) to movement (px/event) based on trigger period
4. Replaces the event value with the calculated movement
5. Resets timing when movement stops or direction changes

## Installation

### As External Module (Recommended)

Add to your `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
  projects:
    - name: zmk-input-processor-speed-curve
      url: https://github.com/N3M0-dev/zmk-input-processor-speed-curve
      revision: main
    - name: zmk
      remote: zmkfirmware
      revision: v0.3.0
      import: app/west.yml
  self:
    path: config
```

Enable in `config/your_keyboard.conf`:

```properties
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_INPUT_PROCESSOR_SPEED_CURVE=y
```

## Configuration

### Basic Example

```devicetree
#include <dt-bindings/zmk/input.h>

/ {
    zip_speed_curve_xy: zip_speed_curve_xy {
        compatible = "zmk,input-processor-speed-curve";
        #input-processor-cells = <0>;
        type = <INPUT_EV_REL>;
        codes = <INPUT_REL_X>, <INPUT_REL_Y>;
        
        // Define your curve: [time_ms, speed_px_per_sec]
        curve-points = <0 50>,      // Start: 50 px/s
                       <300 200>,   // At 300ms: 200 px/s
                       <1000 800>;  // At 1000ms: 800 px/s (max)
        
        trigger-period-ms = <16>;  // ~62.5Hz (match your MMV setting)
        track-remainders;          // Enable sub-pixel accuracy
        status = "okay";
    };
};

&mmv_input_listener {
    input-processors = <&zip_speed_curve_xy>;
};
```

### Device Tree Properties

| Property | Type | Required | Default | Description |
|----------|------|----------|---------|-------------|
| `compatible` | string | Yes | - | Must be `"zmk,input-processor-speed-curve"` |
| `type` | int | Yes | - | Input event type (use `INPUT_EV_REL` for mouse) |
| `codes` | array | Yes | - | Event codes to process (e.g., `INPUT_REL_X`, `INPUT_REL_Y`) |
| `curve-points` | array | Yes | - | Array of [time_ms, speed_px_per_sec] pairs (min 2 points) |
| `trigger-period-ms` | int | No | 16 | Period between events in ms (should match MMV's trigger-period) |
| `track-remainders` | bool | No | false | Enable sub-pixel remainder tracking |

### Curve Examples

**Aggressive Start:**
```devicetree
curve-points = <0 300>,    // Fast initial movement
               <200 400>,  
               <1000 800>;
```

**Slow and Smooth:**
```devicetree
curve-points = <0 20>,
               <500 100>,
               <1500 300>,
               <2000 500>;
```

**Multi-Stage (Fan Curve Style):**
```devicetree
curve-points = <0 50>,     // Idle: minimal movement
               <200 150>,  // Low: slow precision
               <500 400>,  // Medium: normal speed
               <800 600>,  // High: fast movement
               <1000 800>; // Max: full speed
```

**Step Function:**
```devicetree
curve-points = <0 100>,    // Instant speed
               <1000 100>; // Constant throughout
```

## Understanding Speed vs Movement

The processor works with **speed** (pixels per second), which gets converted to **movement** (pixels per event):

```
movement = speed * trigger_period_ms / 1000
```

Examples with 16ms trigger period (~62.5 events/sec):
- 50 px/s → ~0.8 px/event (tracked with remainders)
- 200 px/s → 3.2 px/event
- 800 px/s → 12.8 px/event

## Tips

1. **First point should start at time 0** for immediate response
2. **Match trigger-period-ms** to your `&mmv` behavior's trigger period
3. **Enable track-remainders** for smooth movement at low speeds
4. **Use linear segments** for predictable acceleration feel
5. **Add more points** for finer control over specific speed ranges

## Behavior Notes

- Timer **starts** when movement begins (any non-zero value)
- Timer **resets** when:
  - Movement stops (value becomes 0)
  - Direction changes (positive ↔ negative)
- Speed is **clamped** to first/last point values outside defined time range
- Movement is **always at least 1 pixel** if calculated speed > 0

## Compatibility

- **ZMK Version**: v0.3.0 or newer (requires input processor support)
- **Dependencies**: ZMK pointing support (`CONFIG_ZMK_POINTING=y`)

## Example Full Configuration

```devicetree
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/input.h>
#include <input/processors.dtsi>

// Configure MMV behavior
&mmv {
    trigger-period-ms = <16>;  // ~62.5Hz update rate
};

/ {
    // Define custom speed curve
    zip_speed_curve_xy: zip_speed_curve_xy {
        compatible = "zmk,input-processor-speed-curve";
        #input-processor-cells = <0>;
        type = <INPUT_EV_REL>;
        codes = <INPUT_REL_X>, <INPUT_REL_Y>;
        
        curve-points = <0 50>,
                       <300 200>,
                       <1000 800>;
        
        trigger-period-ms = <16>;
        track-remainders;
        status = "okay";
    };
    
    keymap {
        compatible = "zmk,keymap";
        default_layer {
            bindings = <
                // ... your keys ...
                &mmv MOVE_UP    // Mouse movement with custom curve
                &mmv MOVE_DOWN
                &mmv MOVE_LEFT
                &mmv MOVE_RIGHT
            >;
        };
    };
};

&mmv_input_listener {
    input-processors = <&zip_speed_curve_xy>;
};
```

## Debugging

Enable ZMK logging to see processor activity:

```properties
CONFIG_ZMK_USB_LOGGING=y
CONFIG_ZMK_LOG_LEVEL_DBG=y
```

The processor logs:
- Movement start/stop events
- Direction changes
- Calculated speed and movement values

## License

MIT License - Copyright (c) 2024 The ZMK Contributors

## Contributing

Issues and pull requests welcome at: https://github.com/N3M0-dev/zmk-input-processor-speed-curve
