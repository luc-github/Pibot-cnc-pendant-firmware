/*
  esp3d_serial_config.h

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
#include "driver/uart.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // UART port number
    uart_port_t port;              //UART_NUM_0 or UART_NUM_1 or UART_NUM_2
    
    // Pin configuration
    gpio_num_t rx_pin;             // GPIO pin for RX 
    gpio_num_t tx_pin;             // GPIO pin for TX 
    gpio_num_t rts_pin;            // GPIO pin for RTS (or UART_PIN_NO_CHANGE if not used)
    gpio_num_t cts_pin;            // GPIO pin for CTS (or UART_PIN_NO_CHANGE if not used)
    
    // Standard UART configuration structure from ESP-IDF
    uart_config_t uart_config;     // Reuse the existing ESP-IDF structure
    
    // Buffer sizes for UART driver
    size_t rx_buffer_size;         // From tasks_def.h: ESP3D_SERIAL_RX_BUFFER_SIZE
    size_t tx_buffer_size;         // From tasks_def.h: ESP3D_SERIAL_TX_BUFFER_SIZE
    uint16_t rx_flush_timeout;     // From tasks_def.h: ESP3D_SERIAL_RX_FLUSH_TIMEOUT
    
    // Task configuration
    uint32_t task_priority;        // From tasks_def.h: ESP3D_SERIAL_TASK_PRIORITY
    uint32_t task_stack_size;      // From tasks_def.h: ESP3D_SERIAL_RX_TASK_SIZE
    BaseType_t task_core;          // From tasks_def.h: ESP3D_SERIAL_TASK_CORE
} esp3d_serial_config_t;


#ifdef __cplusplus
} /* extern "C" */
#endif