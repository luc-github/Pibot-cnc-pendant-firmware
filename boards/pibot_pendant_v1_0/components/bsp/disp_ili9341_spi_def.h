/*
  ili9341_def.h - ILI9341 default configuration for PiBot CNC Pendant

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

#include "board_config.h"
#include "disp_ili9341_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default configuration for ILI9341 based on board_config.h
// This creates a constant configuration structure that can be used directly

// Default configuration instance
const spi_ili9341_config_t ili9341_default_config = {
    .spi_bus = {
        .host = DISPLAY_SPI_HOST_IDX,
        .is_master = true,  // Assume this is the master device initializing the SPI bus
        .miso_pin = DISPLAY_MISO_PIN,
        .mosi_pin = DISPLAY_MOSI_PIN,
        .sclk_pin = DISPLAY_CLK_PIN,
        .cs_pin = DISPLAY_CS_PIN,
        .dc_pin = DISPLAY_DC_PIN,
        .rst_pin = DISPLAY_RST_PIN,
        .clock_speed_hz = DISPLAY_SPI_FREQ_HZ,
        .max_transfer_sz = DISPLAY_WIDTH_PX * DISPLAY_BUFFER_LINES_NB * sizeof(uint16_t),
    },
    .dma = {
        .channel = SPI_DMA_CH_AUTO,  // Let ESP-IDF choose the DMA channel
        .quadwp_pin = -1,  // Not using quad SPI
        .quadhd_pin = -1,  // Not using quad SPI
    },
    .backlight = {
        .pin = BACKLIGHT_PIN,
        .on_level = BACKLIGHT_ACTIVE_HIGH_FLAG,
    },
    .display = {
        .width = DISPLAY_WIDTH_PX,
        .height = DISPLAY_HEIGHT_PX,
        .orientation = DISPLAY_ORIENTATION,
        .invert_colors = DISPLAY_INVERT_FLAG,
    },
    .interface = {
        .cmd_bits = 8,  // Standard command size
        .param_bits = 8,  // Standard parameter size
        .swap_color_bytes = DISPLAY_SWAP_COLOR_FLAG,  // Color byte order
    },
    .lvgl = {
        .enable_callbacks = false,
        .user_ctx = NULL,
        .on_color_trans_done = NULL
    }
};

#ifdef __cplusplus
} /* extern "C" */
#endif
