/*
  phy_buttons.h - Physical buttons driver for PiBot CNC Pendant

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include <esp_err.h>
#include "phy_buttons_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the physical buttons
 *
 * This function initializes the GPIO pins for the buttons based on the provided configuration.
 *
 * @param config Pointer to the buttons configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_buttons_configure(const phy_buttons_config_t *config);

/**
 * @brief Read the state of the buttons
 *
 * This function reads the current state of all buttons, applying debounce logic.
 *
 * @param button_states Array to store the state of each button (1 = pressed, 0 = released)
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_buttons_read(bool button_states[3]);

#ifdef __cplusplus
} /* extern "C" */
#endif
