/*
  esp3d_commands member
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
#if ESP3D_BRIGHTNESS_CONTROL_FEATURE
#include "authentication/esp3d_authentication.h"
#include "esp3d_client.h"
#include "esp3d_commands.h"
#include "esp3d_settings.h"
#include "esp3d_string.h"
#include "disp_backlight.h"

#define COMMAND_ID 920
// Display/set brightness 
//[ESP920](brightness) - display/set brightness level (0-100)
void ESP3DCommands::ESP920(int cmd_params_pos, ESP3DMessage* msg) {
  ESP3DClientType target = msg->origin;
  ESP3DRequest requestId = msg->request_id;
  (void)requestId;
  msg->target = target;
  msg->origin = ESP3DClientType::command;
  bool hasError = false;
  std::string error_msg = "Invalid parameters";
  std::string ok_msg = "ok";
  bool json = hasTag(msg, cmd_params_pos, "json");
  std::string tmpstr;
  
#if ESP3D_AUTHENTICATION_FEATURE
  if (msg->authentication_level == ESP3DAuthenticationLevel::guest) {
    dispatchAuthenticationError(msg, COMMAND_ID, json);
    return;
  }
#endif  // ESP3D_AUTHENTICATION_FEATURE
  tmpstr = get_clean_param(msg, cmd_params_pos);
  if (tmpstr.length() == 0) {
   ok_msg = std::to_string(backlight_get_current()) + "%";
  } else {
#if ESP3D_AUTHENTICATION_FEATURE
    if (msg->authentication_level == ESP3DAuthenticationLevel::guest) {
      dispatchAuthenticationError(msg, COMMAND_ID, json);
      return;
    }
#endif  // ESP3D_AUTHENTICATION_FEATURE
   uint8_t brightness = (uint8_t)atoi(tmpstr.c_str());
    if (brightness > 100) {
      hasError = true;
      error_msg = "Invalid brightness value";
      esp3d_log_e("Invalid brightness value");
    } else {
      if (!esp3dTftsettings.writeByte(ESP3DSettingIndex::esp3d_brightness_level, brightness)) {
        hasError = true;
        error_msg = "Failed to set brightness";
        esp3d_log_e("Failed to set brightness");
      } else {
        if (backlight_set(brightness) != ESP_OK) {
          hasError = true;
          error_msg = "Failed to set brightness";
          esp3d_log_e("Failed to set brightness");
        } 
      }
      
    }
  }
  if (!dispatchAnswer(msg, COMMAND_ID, json, hasError,
                      hasError ? error_msg.c_str() : ok_msg.c_str())) {
    esp3d_log_e("Error sending response to clients");
  }
}
#endif  // ESP3D_BRIGHTNESS_CONTROL_FEATURE