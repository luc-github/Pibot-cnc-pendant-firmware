/*
  esp3d_client_types.h

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

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum class ESP3DClientType : uint8_t {
  no_client = 0,
  serial = 1,
  usb_serial = 2,
  stream = 3,
  telnet = 4,
  webui = 5,
  webui_websocket = 6,
  websocket = 7,
  rendering = 8,
  bt_serial,  // Bluetooth Classic Serial
  bt_ble,     // Bluetooth Low Energy
  command,  // origin only
  system,   // origin only
  all_clients
};

#ifdef __cplusplus
}  // extern "C"
#endif