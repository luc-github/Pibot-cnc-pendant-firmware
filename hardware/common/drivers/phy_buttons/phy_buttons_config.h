/*
  phy_buttons_config.h - Physical buttons configuration for PiBot CNC Pendant

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for physical buttons
 *
 * Must be passed to phy_buttons_configure() for correct configuration
 */
typedef struct {
    gpio_num_t button_pins[3];   // Array of GPIO pins for the 3 buttons
    bool pullups_enabled;        // true: Enable internal pull-up resistors, false: Disable
    uint32_t debounce_ms;        // Debounce time in milliseconds
    bool active_low;             // true: Active low (pressed = 0), false: Active high (pressed = 1)
} phy_buttons_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
