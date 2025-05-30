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
#if ESP3D_SD_CARD_FEATURE
#include "board_config.h"
#if SD_INTERFACE_TYPE == 0
#include "authentication/esp3d_authentication.h"
#include "esp3d_client.h"
#include "esp3d_commands.h"
#include "esp3d_settings.h"
#include "esp3d_string.h"
#include "filesystem/esp3d_sd.h"

#define COMMAND_ID 202

// Get/Set SD card Speed factor 1 2 4 6 8 16 32
//[ESP202]<factor> json=<no> pwd=<user/admin password>
void ESP3DCommands::ESP202(int cmd_params_pos, ESP3DMessage* msg) {
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
    ok_msg = std::to_string(
        esp3dTftsettings.readByte(ESP3DSettingIndex::esp3d_spi_divider));
  } else {
#if ESP3D_AUTHENTICATION_FEATURE
    if (msg->authentication_level != ESP3DAuthenticationLevel::admin) {
      dispatchAuthenticationError(msg, COMMAND_ID, json);
      return;
    }
#endif  // ESP3D_AUTHENTICATION_FEATURE
    uint8_t spidiv = atoi(tmpstr.c_str());
    esp3d_log("got %s param for a value of %d, is valid %d", tmpstr.c_str(),
              spidiv,
              esp3dTftsettings.isValidByteSetting(
                  spidiv, ESP3DSettingIndex::esp3d_spi_divider));
    if (esp3dTftsettings.isValidByteSetting(
            spidiv, ESP3DSettingIndex::esp3d_spi_divider)) {
      esp3d_log("Value %d is valid", spidiv);
      if (!esp3dTftsettings.writeByte(ESP3DSettingIndex::esp3d_spi_divider,
                                      spidiv)) {
        hasError = true;
        error_msg = "Set value failed";
      } else {
        sd.setSPISpeedDivider(spidiv);
      }
    } else {
      hasError = true;
      error_msg = "Invalid parameter";
    }
  }

  if (!dispatchAnswer(msg, COMMAND_ID, json, hasError,
                      hasError ? error_msg.c_str() : ok_msg.c_str())) {
    esp3d_log_e("Error sending response to clients");
  }
}
#endif  // SD_INTERFACE_TYPE == 0
#endif  // ESP3D_SD_CARD_FEATURE
