/*
  phy_switch_config.h - 4-position switch configuration for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for 4-position switch
 *
 * Must be passed to phy_switch_configure() for correct configuration
 */
typedef struct {
    gpio_num_t pins[3];          // Array of 3 GPIO pins for the switch
    bool pullups_enabled;        // true: Enable internal pull-up resistors, false: Disable
    uint32_t debounce_ms;        // Debounce time in milliseconds
} phy_switch_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
