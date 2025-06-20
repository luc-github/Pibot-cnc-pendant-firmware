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
#if ESP3D_BUZZER_FEATURE
#include "authentication/esp3d_authentication.h"
#include "buzzer/esp3d_buzzer.h"
#include "esp3d_client.h"
#include "esp3d_commands.h"
#include "esp3d_string.h"


#define COMMAND_ID 250
// Play sound
//[ESP250]F=<frequency> D=<duration> json=<no> pwd=<user password>
void ESP3DCommands::ESP250(int cmd_params_pos, ESP3DMessage *msg)
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
    std::string tmpstr;
    std::string frequency_str = get_param(msg, cmd_params_pos, "F=");
    std::string position_str  = get_param(msg, cmd_params_pos, "D=");
#if ESP3D_AUTHENTICATION_FEATURE
    if (msg->authentication_level == ESP3DAuthenticationLevel::guest)
    {
        dispatchAuthenticationError(msg, COMMAND_ID, json);
        return;
    }
#endif  // ESP3D_AUTHENTICATION_FEATURE
    tmpstr = get_clean_param(msg, cmd_params_pos);
    if (tmpstr.length() == 0)
    {
        hasError  = true;
        error_msg = "Missing parameter";
    }
    else
    {
#if ESP3D_AUTHENTICATION_FEATURE
        if (msg->authentication_level != ESP3DAuthenticationLevel::admin)
        {
            dispatchAuthenticationError(msg, COMMAND_ID, json);
            return;
        }
#endif  // ESP3D_AUTHENTICATION_FEATURE

        int frequency = std::stoi(frequency_str);
        int duration  = std::stoi(position_str);
        if (frequency < 0 || duration < 0)
        {
            hasError  = true;
            error_msg = "Invalid frequency or duration value";
            esp3d_log_e("Invalid frequency or duration value");
        }
        else
        {
            if (!esp3d_buzzer.bip(frequency, duration))
            {
                hasError  = true;
                error_msg = "Failed to play sound";
                esp3d_log_e("Failed to play sound");
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
    }
}
#endif  // ESP3D_BUZZER_FEATURE