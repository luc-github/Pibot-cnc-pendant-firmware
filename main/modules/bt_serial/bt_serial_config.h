/*
  bt_serial_config.h

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
#include "esp_bt.h"
#include "esp_gatt_common_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // Configuration Bluetooth Classic (SPP)
  uint32_t spp_channel;          // Canal SPP (par défaut : 1)
  char device_name[32];          // Nom du périphérique BT (ex. "ESP32_BT")
  bool discoverable;             // Visibilité du périphérique
  bool connectable;              // Peut accepter des connexions
  
  // Buffer sizes
  size_t rx_buffer_size;         // Taille du buffer Rx
  size_t tx_buffer_size;         // Taille du buffer Tx
  uint16_t rx_flush_timeout;     // Timeout pour flush Rx
  
  // Task configuration
  uint32_t task_priority;        // Priorité de la tâche
  uint32_t task_stack_size;      // Taille de la pile
  BaseType_t task_core;          // Cœur pour la tâche
} esp3d_bt_serial_config_t;


#ifdef __cplusplus
} /* extern "C" */
#endif