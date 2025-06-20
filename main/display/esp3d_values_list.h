
/*
  esp3d_values.h -  values esp3d functions class

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with This code; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once
#include <stdio.h>

#include <functional>
#include <list>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// this list depend of target feature
enum class ESP3DValuesIndex : uint16_t
{
#if ESP3D_HAS_STATUS_BAR
    status_bar_label,
#endif  // ESP3D_HAS_STATUS_BAR

#if ESP3D_WIFI_FEATURE || ESP3D_BT_FEATURE
#if ESP3D_WIFI_FEATURE
    current_ip,
#endif  // ESP3D_WIFI_FEATURE
    network_status,
    network_mode,

#endif  // ESP3D_WIFI_FEATURE
    m_position_x,
    m_position_y,
    m_position_z,
#include "esp3d_system_values_list.inc"
#include "esp3d_target_values_list.inc"
    unknown_index
};

#ifdef __cplusplus
}  // extern "C"
#endif