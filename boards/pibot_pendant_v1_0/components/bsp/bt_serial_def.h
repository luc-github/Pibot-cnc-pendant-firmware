/*
  bt_serial_def.h - Bluetooth services definitions for ESP3D-TFT

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#pragma once

#if ESP3D_BT_FEATURE

#include "bt_serial/esp3d_bt_serial_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default configuration for Bluetooth Serial (SPP)
 esp3d_bt_serial_config_t esp3dBTSerialConfig = {
    .spp_channel = 1,
    .device_name = "PibotCNC",
    .discoverable = true,
    .connectable = true,
    .scan_duration = 20, // seconds
    .rx_buffer_size = 512,
    .tx_buffer_size = 512
};
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE