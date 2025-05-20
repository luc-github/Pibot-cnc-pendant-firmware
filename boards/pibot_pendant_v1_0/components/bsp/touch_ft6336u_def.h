/*
  touch_ft6336u_def.h - FT6336U default configuration for PiBot CNC Pendant

  Copyright (c) 2024 Luc LEBOSSE. All rights reserved.

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

#include "board_config.h"
#include "touch_ft6336u_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default configuration instance for FT6336U based on board_config.h
const touch_ft6336u_config_t touch_ft6336u_default_config = {
    .i2c_addr = TOUCH_I2C_ADDR_HEX,
    .i2c_port = TOUCH_I2C_PORT_IDX,
    .i2c_clk_speed = TOUCH_I2C_FREQ_HZ,
    .rst_pin = TOUCH_RST_PIN,
    .int_pin = TOUCH_IRQ_PIN,
    .sda_pin = TOUCH_SDA_PIN,
    .scl_pin = TOUCH_SCL_PIN,
    .swap_xy = TOUCH_SWAP_XY_FLAG,
    .invert_x = TOUCH_MIRROR_X_FLAG,
    .invert_y = TOUCH_MIRROR_Y_FLAG,
    .x_max = DISPLAY_WIDTH_PX,
    .y_max = DISPLAY_HEIGHT_PX
};

#ifdef __cplusplus
}
#endif
