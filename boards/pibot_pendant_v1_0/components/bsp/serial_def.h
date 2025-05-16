/*
  serial_def.h - Serial definitions for ESP3D

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
#include "serial/esp3d_serial_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Serial configuration definition
esp3d_serial_config_t esp3dSerialConfig = {
    // UART port number
    .port = UART_PORT_IDX,
    
    // Pin configuration
    .rx_pin = UART_RX_PIN,
    .tx_pin = UART_TX_PIN,
    .rts_pin = GPIO_NUM_NC,  // Not used with disabled flow control
    .cts_pin = GPIO_NUM_NC,  // Not used with disabled flow control
    
    // Standard UART configuration structure
    .uart_config = {
        .baud_rate = UART_BAUD_RATE_BPS,
        .data_bits = UART_DATA_BITS,
        .parity = UART_PARITY,
        .stop_bits = UART_STOP_BITS,
        .flow_ctrl = UART_FLOW_CTRL,
        .rx_flow_ctrl_thresh = 0,  // Not used with disabled flow control
        .source_clk = UART_SOURCE_CLK,
        .flags = {
            .allow_pd = 0,  // Prevent power down of UART when in sleep mode
            .backup_before_sleep = 0,  // Deprecated, same meaning as allow_pd
        },
    },
    
    // Buffer sizes for UART driver installation
    .rx_buffer_size = UART_RX_BUFFER_SIZE * 2,  // Double size for buffer
    .tx_buffer_size = UART_TX_BUFFER_SIZE,
    .rx_flush_timeout = UART_RX_FLUSH_TIMEOUT,
    
    // Task configuration
    .task_priority = UART_TASK_PRIORITY,
    .task_stack_size = UART_RX_TASK_SIZE,
    .task_core = UART_TASK_CORE
};

#ifdef __cplusplus
} /* extern "C" */
#endif