/*
  esp3d_bt_ble_config.h

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
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
} esp3d_bt_ble_config_t;

// Configuration par défaut (à définir dans bt_services_def.h)
extern esp3d_bt_ble_config_t esp3dBTBleConfig;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE