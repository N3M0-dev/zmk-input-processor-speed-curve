/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_speed_curve

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>

#include <zmk/input_processors/speed_curve.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/**
 * @brief Check if a given code is in the processor's code array
 */
static bool code_matches(const struct zip_speed_curve_config *cfg, uint16_t code) {
    for (size_t i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] == code) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Calculate speed at given elapsed time using piecewise linear interpolation
 * 
 * @param cfg Configuration containing curve points
 * @param elapsed_ms Elapsed time in milliseconds
 * @return Speed in pixels per second
 */
static int32_t calculate_speed(const struct zip_speed_curve_config *cfg, int64_t elapsed_ms) {
    // Curve points are stored as [time0, speed0, time1, speed1, ...]
    size_t num_points = cfg->curve_points_len / 2;
    
    if (num_points == 0) {
        return 0;
    }
    
    // If before first point, use first point's speed
    int32_t first_time = cfg->curve_points[0];
    int32_t first_speed = cfg->curve_points[1];
    
    if (elapsed_ms <= first_time) {
        return first_speed;
    }
    
    // If after last point, use last point's speed
    int32_t last_time = cfg->curve_points[(num_points - 1) * 2];
    int32_t last_speed = cfg->curve_points[(num_points - 1) * 2 + 1];
    
    if (elapsed_ms >= last_time) {
        return last_speed;
    }
    
    // Find the two points to interpolate between
    for (size_t i = 0; i < num_points - 1; i++) {
        int32_t t0 = cfg->curve_points[i * 2];
        int32_t s0 = cfg->curve_points[i * 2 + 1];
        int32_t t1 = cfg->curve_points[(i + 1) * 2];
        int32_t s1 = cfg->curve_points[(i + 1) * 2 + 1];
        
        if (elapsed_ms >= t0 && elapsed_ms <= t1) {
            // Linear interpolation: speed = s0 + (s1 - s0) * (t - t0) / (t1 - t0)
            int64_t time_delta = t1 - t0;
            int64_t speed_delta = s1 - s0;
            int64_t elapsed_in_segment = elapsed_ms - t0;
            
            int32_t speed = s0 + (speed_delta * elapsed_in_segment) / time_delta;
            return speed;
        }
    }
    
    // Should never reach here, but return last speed as fallback
    return last_speed;
}

/**
 * @brief Process input event and apply custom speed curve
 */
static int zip_speed_curve_handle_event(const struct device *dev,
                                         struct input_event *event,
                                         uint32_t param1, uint32_t param2,
                                         struct zmk_input_processor_state *state) {
    const struct zip_speed_curve_config *cfg = dev->config;
    struct zip_speed_curve_data *data = dev->data;

    // Only process matching event types and codes
    if (event->type != cfg->type || !code_matches(cfg, event->code)) {
        return 0;
    }

    int32_t value = event->value;
    
    // Determine if this is X or Y axis (assuming INPUT_REL_X and INPUT_REL_Y)
    bool is_x_axis = (event->code == 0);  // INPUT_REL_X = 0, INPUT_REL_Y = 1
    int8_t *last_direction = is_x_axis ? &data->last_x_direction : &data->last_y_direction;
    
    // Determine current direction
    int8_t current_direction = (value > 0) ? 1 : (value < 0) ? -1 : 0;
    
    // Check if movement stopped (value == 0)
    if (value == 0) {
        *last_direction = 0;
        
        // Check if both axes stopped - if so, reset timing
        if (data->last_x_direction == 0 && data->last_y_direction == 0) {
            data->is_active = false;
            LOG_DBG("Movement stopped, resetting timing");
        }
        return 0;
    }
    
    // Check if direction changed - reset timing
    if (*last_direction != 0 && *last_direction != current_direction) {
        data->is_active = false;
        LOG_DBG("Direction changed, resetting timing");
    }
    
    *last_direction = current_direction;
    
    // Start timing if not already active
    if (!data->is_active) {
        data->start_time = k_uptime_get();
        data->is_active = true;
        LOG_DBG("Movement started at %lld ms", data->start_time);
    }
    
    // Calculate elapsed time
    int64_t current_time = k_uptime_get();
    int64_t elapsed_ms = current_time - data->start_time;
    
    // Calculate speed from curve
    int32_t speed_px_per_sec = calculate_speed(cfg, elapsed_ms);
    
    // Convert speed (px/sec) to movement (px/event)
    // movement = speed * trigger_period_ms / 1000
    int32_t movement = (speed_px_per_sec * cfg->trigger_period_ms) / 1000;
    
    // Ensure at least 1 pixel movement if speed > 0
    if (movement == 0 && speed_px_per_sec > 0) {
        movement = 1;
    }
    
    // Apply direction
    event->value = current_direction * movement;
    
    LOG_DBG("Speed curve: elapsed=%lld ms, speed=%d px/s, movement=%d px/event (original=%d)",
            elapsed_ms, speed_px_per_sec, event->value, value);

    return 0;
}

/**
 * @brief Initialize the speed curve input processor
 */
static int zip_speed_curve_init(const struct device *dev) {
    struct zip_speed_curve_data *data = dev->data;
    
    data->is_active = false;
    data->start_time = 0;
    data->last_x_direction = 0;
    data->last_y_direction = 0;
    
    LOG_DBG("Initialized speed curve input processor: %s", dev->name);
    
    return 0;
}

static const struct zmk_input_processor_driver_api zip_speed_curve_driver_api = {
    .handle_event = zip_speed_curve_handle_event,
};

#define ZIP_SPEED_CURVE_INST(n)                                                         \
    static const uint16_t zip_speed_curve_codes_##n[] = DT_INST_PROP(n, codes);       \
    static const int32_t zip_speed_curve_points_##n[] = DT_INST_PROP(n, curve_points); \
    static const struct zip_speed_curve_config zip_speed_curve_config_##n = {         \
        .type = DT_INST_PROP(n, type),                                                 \
        .codes = zip_speed_curve_codes_##n,                                            \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                       \
        .curve_points = zip_speed_curve_points_##n,                                    \
        .curve_points_len = DT_INST_PROP_LEN(n, curve_points),                         \
        .trigger_period_ms = DT_INST_PROP(n, trigger_period_ms),                       \
    };                                                                                  \
    static struct zip_speed_curve_data zip_speed_curve_data_##n = {};                 \
    DEVICE_DT_INST_DEFINE(n, zip_speed_curve_init, NULL,                              \
                          &zip_speed_curve_data_##n, &zip_speed_curve_config_##n,     \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,            \
                          &zip_speed_curve_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ZIP_SPEED_CURVE_INST)
