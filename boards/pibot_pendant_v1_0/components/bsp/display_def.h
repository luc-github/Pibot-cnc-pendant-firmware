// Display definitions for PiBot CNC Pendant
// Display driver ILI9341 (SPI)
#pragma once

#include "board_config.h"
#include "ili9341.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display configuration using values from board_config.h
ili9341_config_t ili9341_config = {
    .spi_host = DISPLAY_SPI_HOST,
    .pin_mosi = TFT_MOSI,
    .pin_miso = TFT_MISO,
    .pin_clk = TFT_CLK,
    .pin_cs = TFT_CS,
    .pin_dc = TFT_DC,
    .pin_rst = TFT_RST,
    .pclk_hz = DISPLAY_SPI_FREQ,
    .spi_mode = DISPLAY_SPI_MODE,
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
    .color_bits = DISPLAY_COLOR_BITS,
    .orientation = ILI9341_ORIENTATION_LANDSCAPE,
    .swap_color_bytes = DISPLAY_SWAP_COLOR,
    .mirror_x = DISPLAY_MIRROR_X,
    .mirror_y = DISPLAY_MIRROR_Y,
    .invert_colors = DISPLAY_INVERT
};

#ifdef __cplusplus
}
#endif
