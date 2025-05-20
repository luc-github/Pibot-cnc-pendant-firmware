/*
  touch_ft6336u.h

  Copyright (c) 2024 Luc Lebosse. All rights reserved.

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

#include "esp_err.h"
#include "touch_ft6336u_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the FT6336U touch controller
 * 
 * @param config Pointer to configuration structure
 * @return esp_err_t ESP_OK on success, error otherwise
 */
esp_err_t touch_ft6336u_init(const touch_ft6336u_config_t *config);

/**
 * @brief Read touch data from FT6336U
 * 
 * @return touch_ft6336u_data_t Touch data
 */
touch_ft6336u_data_t touch_ft6336u_read(void);

/**
 * @brief Get the maximum X coordinate
 * 
 * @return uint16_t Maximum X coordinate
 */
uint16_t touch_ft6336u_get_x_max(void);

/**
 * @brief Get the maximum Y coordinate
 * 
 * @return uint16_t Maximum Y coordinate
 */
uint16_t touch_ft6336u_get_y_max(void);

#ifdef __cplusplus
}
#endif