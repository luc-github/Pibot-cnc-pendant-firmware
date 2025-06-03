/*
  phy_encoder_config.h - Rotary encoder configuration for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for rotary encoder
 *
 * Must be passed to phy_encoder_configure() for correct configuration
 */
typedef struct {
    gpio_num_t pin_a;            // GPIO pin for phase A
    gpio_num_t pin_b;            // GPIO pin for phase B
    bool pullups_enabled;        // true: Enable internal pull-up resistors, false: Disable
    uint32_t debounce_us;        // Debounce time in microseconds (not used with PCNT)
    uint32_t min_step_interval_us;  // Minimum time between steps in microseconds (not used with PCNT)
    uint32_t steps_per_rev;       // Number of steps per revolution
    int32_t pcnt_high_limit;     // High limit for PCNT counter
    int32_t pcnt_low_limit;      // Low limit for PCNT counter
    uint32_t pcnt_glitch_ns;     // Glitch filter threshold in nanoseconds
} phy_encoder_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif