/*
  phy_switch.h - 4-position switch driver for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include <esp_err.h>
#include "phy_switch_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the 4-position switch
 *
 * This function initializes the switch based on the provided configuration.
 *
 * @param config Pointer to the switch configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_switch_configure(const phy_switch_config_t *config);

/**
 * @brief Read the state of the switch as buttons
 *
 * This function reads the current state of the switch and returns it as a set of button states.
 *
 * @param states Array to store the state of each virtual button (true = pressed, false = released)
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_switch_read(bool *states);

/**
 * @brief Get the current state of the switch
 *
 * This function returns the current key code of the switch (0 to 3).
 *
 * @param key_code Pointer to store the current key code
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_switch_get_state(uint32_t *key_code);

#ifdef __cplusplus
} /* extern "C" */
#endif