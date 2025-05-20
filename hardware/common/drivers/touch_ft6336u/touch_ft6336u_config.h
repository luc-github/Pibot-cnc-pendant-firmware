/*
  touch_ft6336u_config.h

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

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FT6336U touch data structure
 */
typedef struct {
    bool is_pressed;    /*!< Whether the touchscreen is being touched */
    int16_t x;          /*!< X coordinate of the touch point */
    int16_t y;          /*!< Y coordinate of the touch point */
} touch_ft6336u_data_t;

/**
 * @brief FT6336U configuration structure
 */
typedef struct {
    uint8_t i2c_addr;       /*!< I2C address of FT6336U */
    i2c_port_t i2c_port;    /*!< I2C port number */
    int i2c_clk_speed;      /*!< I2C clock speed */
    int8_t rst_pin;         /*!< Reset pin, -1 if not used */
    int8_t int_pin;         /*!< Interrupt pin, -1 if not used */
    int8_t sda_pin;         /*!< SDA pin */
    int8_t scl_pin;         /*!< SCL pin */
    bool swap_xy;           /*!< Swap X and Y coordinates */
    bool invert_x;          /*!< Invert X coordinate */
    bool invert_y;          /*!< Invert Y coordinate */
    uint16_t x_max;         /*!< Maximum X coordinate value */
    uint16_t y_max;         /*!< Maximum Y coordinate value */
} touch_ft6336u_config_t;

#ifdef __cplusplus
}
#endif
