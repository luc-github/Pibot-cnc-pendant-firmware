/*
  phy_encoder.h - Rotary encoder driver for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#pragma once

#include <esp_err.h>
#include "phy_encoder_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the rotary encoder
 *
 * This function initializes the GPIO pins for the encoder based on the provided configuration.
 *
 * @param config Pointer to the encoder configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_configure(const phy_encoder_config_t *config);

/**
 * @brief Read the encoder state
 *
 * This function reads the current state of the encoder and returns the number of steps (positive for clockwise, negative for counterclockwise).
 *
 * @param steps Pointer to store the number of steps since the last read
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_read(int32_t *steps);

#ifdef __cplusplus
} /* extern "C" */
#endif