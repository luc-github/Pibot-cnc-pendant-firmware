/*
  backlight.h

  Copyright (c) 2023 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include <esp_err.h>
#include <stdbool.h>
#include "backlight_config.h"

#ifdef __cplusplus
extern "C" { /* extern "C" */
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Creates a display backlight instance.
 *
 * This function creates a display backlight instance based on the provided
 * configuration.
 *
 * @param config Pointer to the backlight configuration.
 * @param bckl Pointer to the created backlight instance.
 * @return `ESP_OK` if the backlight instance is created successfully, or an
 * error code if it fails.
 */
esp_err_t backlight_configure(backlight_config_t *config);

/**
 * @brief Sets the brightness of the display backlight.
 *
 * This function sets the brightness of the display backlight to the specified
 * percentage.
 *
 * @param brightness_percent The brightness level to set, specified as a
 * percentage (0-100).
 *
 * @return `ESP_OK` if the brightness was set successfully, or an error code if
 * an error occurred.
 */
esp_err_t backlight_set(int brightness_percent);



#ifdef __cplusplus
} /* extern "C" */
#endif
