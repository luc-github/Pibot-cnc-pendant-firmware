/*
  phy_potentiometer.h - Potentiometer driver for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include <esp_err.h>
#include "phy_potentiometer_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the potentiometer
 *
 * This function initializes the ADC for the potentiometer based on the provided configuration.
 *
 * @param config Pointer to the potentiometer configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_potentiometer_configure(const phy_potentiometer_config_t *config);

/**
 * @brief Read the potentiometer value
 *
 * This function reads the current ADC value and applies filtering if enabled.
 *
 * @param value Pointer to store the filtered ADC value (0-4095 for 12 bits)
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_potentiometer_read(uint32_t *value);

#ifdef __cplusplus
} /* extern "C" */
#endif