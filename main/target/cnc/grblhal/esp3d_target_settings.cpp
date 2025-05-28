
/*
  esp3d_target_settings.cpp -  settings esp3d functions class

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
#include "esp3d_settings.h"

#include "nvs_flash.h"
#include "nvs_handle.hpp"

#if ESP3D_WIFI_FEATURE
#include "lwip/ip_addr.h"
#endif  // ESP3D_WIFI_FEATURE
#include <cstring>
#include <regex>
#include <string>

#include "esp3d_client.h"
#include "esp3d_log.h"
#include "esp_system.h"
#include "board_config.h"

#include "network/esp3d_network.h"

#include "authentication/esp3d_authentication.h"
#include "board_init.h"


// Is Valid String Setting ?
bool ESP3DSettings::isValidStringTargetSetting(const char* value,
                                         ESP3DSettingIndex settingElement) {
  switch (settingElement) {
    default:
      return false;
  }
  return false;
}

// Is Valid Integer Setting ?
bool ESP3DSettings::isValidIntegerTargetSetting(uint32_t value,
                                          ESP3DSettingIndex settingElement) {
   switch (settingElement) {
    default:
      return false;
  }
  return false;
}

// Is Valid Byte Setting ?
bool ESP3DSettings::isValidByteTargetSetting(uint8_t value,
                                       ESP3DSettingIndex settingElement) {
   switch (settingElement) {
    default:
      return false;
  }
  return false;
}

