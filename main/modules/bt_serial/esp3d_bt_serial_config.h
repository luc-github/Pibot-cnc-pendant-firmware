/*
  esp3d_bt_serial_config.h

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

#if ESP3D_BT_FEATURE

#include "esp_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // Configuration Bluetooth Classic (SPP)
  uint32_t spp_channel;          // Canal SPP (par défaut : 1)
  char device_name[32];          // Nom du périphérique BT (ex. "PibotCNC")
  bool discoverable;             // Visibilité du périphérique
  bool connectable;              // Peut accepter des connexions
  uint16_t scan_duration;        // Durée du scan en secondes (ex. 10)

  // Buffer sizes
  size_t rx_buffer_size;         // Taille du buffer Rx (ex. 1024)
  size_t tx_buffer_size;         // Taille du buffer Tx (ex. 1024)
} esp3d_bt_serial_config_t;

// Configuration par défaut (à définir dans bt_services_def.h)
extern esp3d_bt_serial_config_t esp3dBTSerialConfig;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE