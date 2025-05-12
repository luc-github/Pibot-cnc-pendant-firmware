// Touch controller definitions for PiBot CNC Pendant
// Touch controller: FT6336U (I2C)
#pragma once

#include "board_config.h"
#include "ft6336u.h"

#ifdef __cplusplus
extern "C" {
#endif

// Touch controller configuration using values from board_config.h
ft6336u_config_t ft6336u_config = {
    .i2c_port = TOUCH_I2C_PORT,
    .i2c_address = TOUCH_I2C_ADDR,
    .sda_pin = TOUCH_SDA,
    .scl_pin = TOUCH_SCL,
    .rst_pin = TOUCH_RST,
    .int_pin = TOUCH_IRQ,
    .i2c_freq = TOUCH_I2C_FREQ,
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
    .swap_xy = TOUCH_SWAP_XY,
    .invert_x = TOUCH_MIRROR_X,
    .invert_y = TOUCH_MIRROR_Y
};

#ifdef __cplusplus
}
#endif
