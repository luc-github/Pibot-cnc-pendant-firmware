/*
  board_init.h - PiBot CNC Pendant hardware initialization

  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.

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
#include "lvgl.h"
#include <sys/lock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all board hardware and subsystems
 * 
 * This function initializes all hardware components and subsystems:
 * - SPI/I2C buses configuration
 * - GPIO pins setup
 * - Display and touch panel initialization
 * - Backlight control
 * - Input devices (encoder, buttons, position switch)
 * - SD card (if present)
 * - Communication interfaces (Serial, BT, etc.)
 * - LVGL graphics library
 * 
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t board_init(void);

/**
 * @brief Get board name
 * 
 * @return const char* Board name string
 */
const char* board_get_name(void);

/**
 * @brief Get board version
 * 
 * @return const char* Board version string
 */
const char* board_get_version(void);

/**
 * @brief Get the LVGL display handle
 * 
 * @return lv_display_t* Display handle or NULL if not initialized
 */
lv_display_t* get_lvgl_display(void);

/**
 * @brief Get LVGL API lock for thread-safe access
 * 
 * @return _lock_t* Lock handle
 */
_lock_t* get_lvgl_lock(void);

#ifdef __cplusplus
}
#endif