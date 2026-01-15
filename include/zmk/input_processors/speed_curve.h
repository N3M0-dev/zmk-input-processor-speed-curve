/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

/**
 * @brief Configuration for speed curve input processor
 */
struct zip_speed_curve_config {
    uint8_t type;                   // Input event type (e.g., INPUT_EV_REL)
    const uint16_t *codes;          // Array of event codes to process
    size_t codes_len;               // Number of codes
    const int32_t *curve_points;    // Array of [time_ms, speed] pairs
    size_t curve_points_len;        // Number of curve points (pairs)
    uint16_t trigger_period_ms;     // Period between events in ms
    bool track_remainders;          // Whether to track sub-pixel remainders
};

/**
 * @brief Runtime data for speed curve input processor
 */
struct zip_speed_curve_data {
    int64_t x_start_time;           // Timestamp when X axis movement started (uptime_get())
    int64_t y_start_time;           // Timestamp when Y axis movement started (uptime_get())
    int8_t last_x_direction;        // Last X direction: -1, 0, 1
    int8_t last_y_direction;        // Last Y direction: -1, 0, 1
    float x_remainder;              // Sub-pixel remainder for X axis
    float y_remainder;              // Sub-pixel remainder for Y axis
};
