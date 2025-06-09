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
#if defined(ESP3D_BUZZER_FEATURE) && ESP3D_BUZZER_FEATURE == 1
#include "authentication/esp3d_authentication.h"
#include "buzzer/esp3d_buzzer.h"
#include "esp3d_client.h"
#include "esp3d_commands.h"
#include "esp3d_settings.h"
#include "esp3d_string.h"

#define COMMAND_ID 910

// Get state / Set Enable / Disable buzzer
//[ESP910]<ENABLE/DISABLE> json=<no> pwd=<admin/user password>
void ESP3DCommands::ESP910(int cmd_params_pos, ESP3DMessage *msg)
{
    ESP3DClientType target = msg->origin;
    ESP3DRequest requestId = msg->request_id;
    (void)requestId;
    msg->target           = target;
    msg->origin           = ESP3DClientType::command;
    bool hasError         = false;
    std::string error_msg = "Invalid parameters";
    std::string ok_msg    = "ok";
    bool json             = hasTag(msg, cmd_params_pos, "json");
    bool setEnable        = hasTag(msg, cmd_params_pos, "ENABLE");
    bool setDisable       = hasTag(msg, cmd_params_pos, "DISABLE");
    std::string tmpstr;
    tmpstr = get_clean_param(msg, cmd_params_pos);

    if (tmpstr.length() == 0)
    {
        uint8_t b = esp3dTftsettings.readByte(ESP3DSettingIndex::esp3d_buzzer_on);
        if (b == (uint8_t)ESP3DState::on)
        {
            ok_msg = "ENABLED";
        }
        else
        {
            ok_msg = "DISABLED";
        }
    }
    else
    {
#if ESP3D_AUTHENTICATION_FEATURE
        if (msg->authentication_level == ESP3DAuthenticationLevel::guest)
        {
            dispatchAuthenticationError(msg, COMMAND_ID, json);
            return;
        }
#endif  // ESP3D_AUTHENTICATION_FEATURE
        if (!(setEnable || setDisable) && !has_param(msg, cmd_params_pos))
        {  // get
            hasError = true;
        }
        else
        {
            if (!esp3dTftsettings.writeByte(ESP3DSettingIndex::esp3d_buzzer_on,
                                            setEnable ? (uint8_t)ESP3DState::on
                                                      : (uint8_t)ESP3DState::off))
            {
                hasError  = true;
                error_msg = "Failed to set buzzer state";
            }
            else
            {
                esp3d_buzzer.begin();
            }
        }
    }
    if (!dispatchAnswer(msg,
                        COMMAND_ID,
                        json,
                        hasError,
                        hasError ? error_msg.c_str() : ok_msg.c_str()))
    {
        esp3d_log_e("Error sending response to clients");
        return;
    }
}
#endif  // ESP3D_BUZZER_FEATURE