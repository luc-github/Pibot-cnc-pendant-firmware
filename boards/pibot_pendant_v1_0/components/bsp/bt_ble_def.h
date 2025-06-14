/*
  bt_ble_def.h - Bluetooth services definitions for ESP3D-TFT

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#pragma once

#if ESP3D_BT_FEATURE

#include "bt_ble/esp3d_bt_ble_config.h"

#ifdef __cplusplus
extern "C" {
#endif
// Default configuration for Bluetooth Low Energy (BLE)
 esp3d_bt_ble_config_t esp3dBTBleConfig = {
    .device_name = "PibotCNC",
    .service_uuid = 0x1800,  // À personnaliser selon votre service GATT
    .char_uuid = 0x2A00,     // À personnaliser selon votre caractéristique
    .advertise = true,
    .scan_duration = 10,
    .rx_buffer_size = 1024,
    .tx_buffer_size = 1024
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE