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
#if ESP3D_WIFI_FEATURE || ESP3D_BT_FEATURE
#    include "authentication/esp3d_authentication.h"
#    include "esp3d_client.h"
#    include "esp3d_commands.h"
#    include "esp3d_settings.h"
#    include "esp3d_string.h"

#    define COMMAND_ID 101
// STA Password
//[ESP101]<Password> <NOPASSWORD>  WIFI=<Password> / BTSERIAL=<Password> / BTBLE=<Password>  json=no
// pwd=<admin password>
void ESP3DCommands::ESP101(int cmd_params_pos, ESP3DMessage *msg)
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
   
#    if ESP3D_WIFI_FEATURE
    bool clearSetting     = hasTag(msg, cmd_params_pos, "NOPASSWORD");
    std::string wifi_password = get_param(msg, cmd_params_pos, "WIFI=");
    if (wifi_password.length() == 0)
    {
        wifi_password = get_clean_param(msg, cmd_params_pos);
    }
#    endif  // ESP3D_WIFI_FEATURE
#    if ESP3D_BT_FEATURE
    std::string btserial_password = get_param(msg, cmd_params_pos, "BTSERIAL=");
    std::string btble_password    = get_param(msg, cmd_params_pos, "BTBLE=");
#    endif  // ESP3D_BT_FEATURE
    std::string tmpstr;
#    if ESP3D_AUTHENTICATION_FEATURE
    if (msg->authentication_level != ESP3DAuthenticationLevel::admin)
    {
        dispatchAuthenticationError(msg, COMMAND_ID, json);
        return;
    }
#    endif  // ESP3D_AUTHENTICATION_FEATURE
    tmpstr = get_clean_param(msg, cmd_params_pos);
    if (tmpstr.length() == 0)
    {
        hasError  = true;
        error_msg = "Password not displayable";
    }
    else
    {
#    if ESP3D_WIFI_FEATURE
  if (clearSetting || wifi_password == "NOPASSWORD" ||)
        {
            esp3d_log("NOPASSWORD flag detected, set string to empty string");
            wifi_password = "";
        }
         if (esp3dTftsettings.isValidStringSetting(wifi_password.c_str(),
                                                  ESP3DSettingIndex::esp3d_sta_password))
        {
            esp3d_log("Value %s is valid", wifi_password.c_str());
            if (!esp3dTftsettings.writeString(ESP3DSettingIndex::esp3d_sta_password,
                                              wifi_password.c_str()))
            {
                hasError  = true;
                error_msg = "Set value failed";
            }
        }
        else
        {
            hasError  = true;
            error_msg = "Invalid parameter";
        }
#    endif  // ESP3D_WIFI_FEATURE
     
#    if ESP3D_BT_FEATURE
     if(btserial_password == "NOPASSWORD" )
        {
            esp3d_log("NOPASSWORD flag detected, set string to empty string");
            btserial_password = "";
        }
        if (esp3dTftsettings.isValidStringSetting(btserial_password.c_str(),
                                                  ESP3DSettingIndex::esp3d_btserial_pin))
        {
            esp3d_log("Value %s is valid", btserial_password.c_str());
            if (!esp3dTftsettings.writeString(ESP3DSettingIndex::esp3d_btserial_pin,
                                              btserial_password.c_str()))
            {
                hasError  = true;
                error_msg = "Set value failed";
            }
        }
        else
        {
            hasError  = true;
            error_msg = "Invalid parameter";
        }
        
        if(btble_password == "NOPASSWORD" )
        {
            esp3d_log("NOPASSWORD flag detected, set string to empty string");
            btble_password = "";
        }
        if (esp3dTftsettings.isValidStringSetting(btble_password.c_str(),
                                                  ESP3DSettingIndex::esp3d_btble_passkey))
        {
            esp3d_log("Value %s is valid", btble_password.c_str());
            if (!esp3dTftsettings.writeString(ESP3DSettingIndex::esp3d_btble_passkey,
                                              btble_password.c_str()))
            {
                hasError  = true;
                error_msg = "Set value failed";
            }
        }
        else
        {
            hasError  = true;
            error_msg = "Invalid parameter";
        }
#    endif  // ESP3D_BT_FEATURE
        
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
#endif  // ESP3D_WIFI_FEATURE