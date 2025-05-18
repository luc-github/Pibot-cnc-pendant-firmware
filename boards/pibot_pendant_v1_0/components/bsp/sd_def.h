/*
  sd_def.h - SD Card definitions for ESP3D-TFT

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
#include "filesystem/esp3d_sd_config.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Default SPI transfer and allocation sizes if not defined in board config
#ifndef SD_MAX_TRANSFER_SIZE
#define SD_MAX_TRANSFER_SIZE 4096
#endif

#ifndef SD_ALLOCATION_SIZE
#define SD_ALLOCATION_SIZE (4 * 1024)
#endif

// Default SPI speed divider if not defined in board config
#ifndef SD_SPI_SPEED_DIVIDER
#define SD_SPI_SPEED_DIVIDER 1
#endif


// Board-specific SD configuration
#if SD_INTERFACE_TYPE == ESP3D_SD_INTERFACE_SDIO

    // Define SD card as SDIO interface
    esp3d_sd_config_t esp3dSdConfig = {
        .interface_type = ESP3D_SD_INTERFACE_SDIO,
        .detect_pin = SD_DETECT_PIN,
        .detect_value = SD_DETECT_VALUE,
        .freq = SD_SDIO_FREQ_HZ,     //  Hz
        .sdio = {
            .cmd_pin = SD_CMD_PIN,
            .clk_pin = SD_CLK_PIN,
            .d0_pin = SD_D0_PIN,
            .d1_pin = SD_D1_PIN,
            .d2_pin = SD_D2_PIN,
            .d3_pin = SD_D3_PIN,
            .bit_width = SD_BIT_WIDTH
        }
    };
#else
    // Define SD card as SPI interface
    #include "driver/sdspi_host.h"
    esp3d_sd_config_t esp3dSdConfig = {
        .interface_type = ESP3D_SD_INTERFACE_SPI,
        .detect_pin = SD_DETECT_PIN,
        .detect_value = SD_DETECT_VALUE,
        .freq = SD_SPI_FREQ_HZ,      //  Hz
        .spi = {
            .mosi_pin = SD_MOSI_PIN,
            .miso_pin = SD_MISO_PIN,
            .clk_pin = SD_CLK_PIN,
            .cs_pin = SD_CS_PIN,
            .host = SD_SPI_HOST_IDX,
            .speed_divider = SD_SPI_SPEED_DIVIDER,
            .max_transfer_sz = SD_MAX_TRANSFER_SIZE,
            .allocation_size = SD_ALLOCATION_SIZE
        }
    };
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
