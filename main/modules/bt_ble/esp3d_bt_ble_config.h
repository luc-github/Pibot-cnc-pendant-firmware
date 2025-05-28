/*
  esp3d_bt_ble_config.h

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

#if ESP3D_BT_FEATURE

#include "esp_bt.h"
#include "esp_gatt_common_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // Configuration BLE
  char device_name[32];          // Nom du périphérique BLE (ex. "PibotCNC")
  uint16_t service_uuid;         // UUID du service GATT personnalisé (16 bits)
  uint16_t char_uuid;            // UUID de la caractéristique texte (16 bits)
  bool advertise;                // Activer l'advertising BLE
  uint16_t scan_duration;        // Durée du scan en secondes (ex. 10)

  // Buffer sizes
  size_t rx_buffer_size;         // Taille du buffer Rx (ex. 1024)
  size_t tx_buffer_size;         // Taille du buffer Tx (ex. 1024)
  uint16_t rx_flush_timeout;     // Timeout pour flush Rx (ex. 100 ms)

  // Task configuration
  uint32_t task_priority;        // Priorité de la tâche FreeRTOS (ex. 5)
  uint32_t task_stack_size;      // Taille de la pile (ex. 4096)
  BaseType_t task_core;          // Cœur pour la tâche (ex. 1)
} esp3d_bt_ble_config_t;

// Configuration par défaut (à définir dans board_config.h)
extern esp3d_bt_ble_config_t esp3dBTBleConfig;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE