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
 * This function initializes the GPIO pins for the switch based on the provided configuration.
 *
 * @param config Pointer to the switch configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_switch_configure(const phy_switch_config_t *config);

/**
 * @brief Read the state of the switch
 *
 * This function reads the current state of the switch and returns a key code.
 *
 * @param key_code Pointer to store the key code (0-3 for the 4 states)
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_switch_read(uint32_t *key_code);

#ifdef __cplusplus
} /* extern "C" */
#endif
