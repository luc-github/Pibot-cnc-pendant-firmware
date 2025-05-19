/*
  disp_ili9341_config.h

  Copyright (c) 2022 Luc Lebosse. All rights reserved.

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
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Orientation options for ILI9341 display
 */
typedef enum {
    SPI_ILI9341_ORIENTATION_PORTRAIT         = 0, // 0 degree rotation
    SPI_ILI9341_ORIENTATION_LANDSCAPE        = 1, // 90 degree rotation
    SPI_ILI9341_ORIENTATION_PORTRAIT_INVERTED = 2, // 180 degree rotation
    SPI_ILI9341_ORIENTATION_LANDSCAPE_INVERTED = 3, // 270 degree rotation
} spi_ili9341_orientation_t;

/**
 * @brief Configuration structure for ILI9341 display over SPI
 */
typedef struct {
    /* SPI configuration */
    struct {
        int host;                /*!< SPI host index (SPI2_HOST, etc.) */
        bool is_master;          /*!< true: initialize SPI bus, false: bus already initialized */
        int miso_pin;            /*!< GPIO pin for MISO */
        int mosi_pin;            /*!< GPIO pin for MOSI */
        int sclk_pin;            /*!< GPIO pin for SCLK (clock) */
        int cs_pin;              /*!< GPIO pin for CS (chip select) */
        int dc_pin;              /*!< GPIO pin for DC (data/command) */
        int rst_pin;             /*!< GPIO pin for RST (reset) */
        uint32_t clock_speed_hz; /*!< SPI clock frequency in Hz */
        int max_transfer_sz;     /*!< Maximum transfer size in bytes */
    } spi_bus;
    
    /* DMA configuration */
    struct {
        int channel;             /*!< DMA channel (SPI_DMA_CH_AUTO, etc.) */
        int quadwp_pin;          /*!< Quad write pin number, -1 if not used */
        int quadhd_pin;          /*!< Quad read pin number, -1 if not used */
    } dma;
    
    /* Backlight configuration */
    struct {
        int pin;                 /*!< GPIO pin for backlight control */
        int on_level;            /*!< Logic level to turn backlight on (0 or 1) */
    } backlight;
    
    /* Display properties */
    struct {
        int width;               /*!< Horizontal resolution in pixels */
        int height;              /*!< Vertical resolution in pixels */
        spi_ili9341_orientation_t orientation; /*!< Display orientation */
        bool invert_colors;      /*!< Whether to invert display colors */
    } display;
    
    /* Command interface configuration */
    struct {
        uint8_t cmd_bits;        /*!< Bit number used to represent command (usually 8) */
        uint8_t param_bits;      /*!< Bit number used to represent parameter (usually 8) */
        bool swap_color_bytes;   /*!< Whether to swap RGB bytes (ILI9341 usually needs this) */
    } interface;
    
    /* LVGL integration */
    struct {
        bool enable_callbacks;                      /*!< Enable LVGL callbacks */
        void *user_ctx;                             /*!< User context for callbacks */
        esp_lcd_panel_io_color_trans_done_cb_t on_color_trans_done; /*!< Callback when color transaction done */
    } lvgl;
    
} spi_ili9341_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
