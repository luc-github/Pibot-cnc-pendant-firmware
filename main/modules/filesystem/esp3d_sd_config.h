/*
  esp3d_sd_config.h

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
// Interface type definitions
#define ESP3D_SD_INTERFACE_SPI   0
#define ESP3D_SD_INTERFACE_SDIO  1

// SD Card Configuration Structure
typedef struct {
    // Interface type - determines which part of the union is valid
    uint8_t interface_type;      // ESP3D_SD_INTERFACE_SPI (0) or ESP3D_SD_INTERFACE_SDIO (1)
    
    // Common configuration
    gpio_num_t detect_pin;       // Card detect pin (GPIO_NUM_NC if not used)
    uint8_t detect_value;        // Card detect value (0 = LOW, 1 = HIGH)
    uint32_t freq;               // Frequency in kHz (20000 for SPI, 40000 for SDIO)
    
    // Interface-specific configuration stored in a union to save memory
    union {
        // SPI specific configuration
        struct {
            gpio_num_t mosi_pin;       // MOSI pin
            gpio_num_t miso_pin;       // MISO pin
            gpio_num_t clk_pin;        // CLK pin
            gpio_num_t cs_pin;         // CS pin
            uint8_t host;              // SPI host (SPI2_HOST or SPI3_HOST if not ESP32, if ESP32: HSPI_HOST or VSPI_HOST)
            uint8_t speed_divider;     // SPI speed divider (used to adjust frequency)
            int max_transfer_sz;  // Max transfer size
            uint32_t allocation_size;  // Allocation size
        } spi;
        
        // SDIO specific configuration
        struct {
            gpio_num_t cmd_pin;        // CMD pin
            gpio_num_t clk_pin;        // CLK pin
            gpio_num_t d0_pin;         // D0 pin
            gpio_num_t d1_pin;         // D1 pin
            gpio_num_t d2_pin;         // D2 pin
            gpio_num_t d3_pin;         // D3 pin
            uint8_t bit_width;         // Bit width
        } sdio;
    };
} esp3d_sd_config_t;


#ifdef __cplusplus
} /* extern "C" */
#endif