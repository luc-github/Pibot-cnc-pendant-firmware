/*
  disp_ili9341_spi.h

  Copyright (c) 2025 Luc Lebosse. All rights reserved.

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

#include <esp_err.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"  // Ajout de l'include pour esp_lcd_panel_io_handle_t
#include "disp_ili9341_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the ILI9341 SPI display
 *
 * This function initializes the SPI bus and configures the ILI9341 display.
 *
 * @param config Pointer to the display configuration structure
 * @return ESP_OK on success, or an error code if initialization fails
 */
esp_err_t ili9341_spi_configure(const spi_ili9341_config_t *config);

/**
 * @brief Get the LCD panel handle
 *
 * This function returns the ESP LCD panel handle for use with LVGL
 * or other rendering libraries. Must be called after ili9341_spi_configure().
 *
 * @return The LCD panel handle, or NULL if not initialized
 */
esp_lcd_panel_handle_t ili9341_spi_get_panel_handle(void);

/**
 * @brief Get the LCD panel IO handle
 *
 * This function returns the ESP LCD panel IO handle for use with LVGL
 * to register event callbacks. Must be called after ili9341_spi_configure().
 *
 * @return The LCD panel IO handle, or NULL if not initialized
 */
esp_lcd_panel_io_handle_t ili9341_spi_get_io_handle(void);

#ifdef __cplusplus
}
#endif