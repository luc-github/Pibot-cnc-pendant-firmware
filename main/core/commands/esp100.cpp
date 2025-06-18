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
#include "authentication/esp3d_authentication.h"
#include "esp3d_client.h"
#include "esp3d_commands.h"
#include "esp3d_settings.h"
#include "esp3d_string.h"

#define COMMAND_ID 100
// Set/Get STA SSID
// output is JSON or plain text according parameter
//[ESP100]<SSID> /  WIFI=<SSID> / BTSERIAL=<BTID> / BTBLE=<BTID> json=<no> pwd=<admin password for
// set/get & user password to, if DISCONNECT or NONE are used the current entry is removed

void ESP3DCommands::ESP100(int cmd_params_pos, ESP3DMessage *msg)
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
    char out_str[255] = {0};
#if ESP3D_AUTHENTICATION_FEATURE
    if (msg->authentication_level == ESP3DAuthenticationLevel::guest)
    {
        dispatchAuthenticationError(msg, COMMAND_ID, json);
        return;
    }
#endif  // ESP3D_AUTHENTICATION_FEATURE
    tmpstr = get_clean_param(msg, cmd_params_pos);
#if ESP3D_WIFI_FEATURE
    std::string wifi_id = get_param(msg, cmd_params_pos, "WIFI=");
    if (wifi_id.length() == 0)
    {
        wifi_id = get_clean_param(msg, cmd_params_pos);
    }
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_BT_FEATURE
    std::string btserial_address = get_param(msg, cmd_params_pos, "BTSERIAL=");
    std::string btble_id         = get_param(msg, cmd_params_pos, "BTBLE=");
#endif  // ESP3D_BT_FEATURE
    // it is query without parameter
    if (tmpstr.length() == 0)
    {
        if (json)
        {
            ok_msg = "{";
        }
        else
        {
            ok_msg = "";
        }
        const ESP3DSettingDescription *settingPtr = NULL;
#if ESP3D_WIFI_FEATURE
        settingPtr = esp3dTftsettings.getSettingPtr(ESP3DSettingIndex::esp3d_sta_ssid);
        if (settingPtr)
        {
            tmpstr = esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_sta_ssid,
                                                 out_str,
                                                 settingPtr->size);
        }
        else
        {
            hasError  = true;
            error_msg = "This setting is unknown";
            tmpstr    = "???";
        }
        if (json)
        {
            ok_msg += "\"ssid\":\"";
        }
        else
        {
            ok_msg += "SSID: ";
        }
        ok_msg += tmpstr;
        if (json)
        {
            ok_msg += "\"";
        }
        else
        {
            ok_msg += "\n";
        }

#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_BT_FEATURE
        settingPtr = esp3dTftsettings.getSettingPtr(ESP3DSettingIndex::esp3d_btserial_address);
        if (settingPtr)
        {
            tmpstr = esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_btserial_address,
                                                 out_str,
                                                 settingPtr->size);
        }
        else
        {
            hasError  = true;
            error_msg = "This setting is unknown";
            tmpstr    = "???";
        }
        if (json)
        {
            if (ok_msg.length() > 1)
            {
                ok_msg += ",";
            }
            ok_msg += "\"btserial_address\":\"";
        }
        else
        {
            ok_msg += "BTSERIAL: ";
        }
        ok_msg += tmpstr;
        if (json)
        {
            ok_msg += "\"";
        }
        else
        {
            ok_msg += "\n";
        }

        settingPtr = esp3dTftsettings.getSettingPtr(ESP3DSettingIndex::esp3d_btble_id);
        if (settingPtr)
        {
            tmpstr = esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_btble_id,
                                                 out_str,
                                                 settingPtr->size);
        }
        else
        {
            hasError  = true;
            error_msg = "This setting is unknown";
            tmpstr    = "???";
        }
        if (json)
        {
            ok_msg += ",\"btble_id\":\"";
        }
        else
        {
            ok_msg += "BTBLE: ";
        }
        ok_msg += tmpstr;
#endif  // ESP3D_WIFI_FEATURE

        if (json)
        {
            ok_msg += "}";
        }
        else
        {
            ok_msg += "\n";
        }
    }
    else
    {  // it is a set command
#if ESP3D_AUTHENTICATION_FEATURE
        if (msg->authentication_level != ESP3DAuthenticationLevel::admin)
        {
            dispatchAuthenticationError(msg, COMMAND_ID, json);
            return;
        }
#endif  // ESP3D_AUTHENTICATION_FEATURE
int count = 0;
#if ESP3D_WIFI_FEATURE
        if (wifi_id.length() != 0)
        {
            count++;
            esp3d_log_d("Save ssid");
            if (esp3dTftsettings.isValidStringSetting(wifi_id.c_str(),
                                                      ESP3DSettingIndex::esp3d_sta_ssid)
                || wifi_id == "NONE" || wifi_id == "DISCONNECT")
            {
                esp3d_log_d("Value %s is valid", tmpstr.wifi_id());
                // if wifi_id is NONE or DISCONNECT, it will be saved as empty string
                if (wifi_id == "NONE" || wifi_id == "DISCONNECT")
                {
                    esp3d_log_d("Set ssid to empty string");
                    wifi_id = " ";
                }
                else
                {
                    esp3d_log_d("Set ssid to %s", wifi_id.c_str());
                }
                if (!esp3dTftsettings.writeString(ESP3DSettingIndex::esp3d_sta_ssid,
                                                  wifi_id.c_str()))
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
        }
#endif  // ESP3D_WIFI_FEATURE

#if ESP3D_BT_FEATURE
        if (btserial_address.length() != 0)
        {
            count++;
            esp3d_log_d("Save btserial");
            if (esp3dTftsettings.isValidStringSetting(btserial_address.c_str(),
                                                      ESP3DSettingIndex::esp3d_btserial_address)
                || btserial_address == "NONE" || btserial_address == "DISCONNECT")
            {
                esp3d_log_d("Value %s is valid", btserial_address.c_str());
                // if btserial_address is NONE or DISCONNECT, it will be saved as empty string
                if (btserial_address == "NONE" || btserial_address == "DISCONNECT")
                {
                    esp3d_log_d("Set btserial address to empty string");
                    btserial_address = " ";
                }
                else
                {
                    esp3d_log_d("Set btserial address to %s", btserial_address.c_str());
                }
                if (!esp3dTftsettings.writeString(ESP3DSettingIndex::esp3d_btserial_address,
                                                  btserial_address.c_str()))
                {
                    hasError  = true;
                    error_msg = "Set value failed";
                }
                else
                {
                    esp3d_log_d("btserial address saved");
                }
            }
            else
            {
                hasError  = true;
                error_msg = "Invalid parameter";
            }
        }
        if (btble_id.length() != 0)
        {
            count++;
            esp3d_log_d("Save btble");
            if (esp3dTftsettings.isValidStringSetting(btble_id.c_str(),
                                                      ESP3DSettingIndex::esp3d_btble_id))
            {
                esp3d_log_d("Value %s is valid", btble_id.c_str());
                if (!esp3dTftsettings.writeString(ESP3DSettingIndex::esp3d_btble_id,
                                                  btble_id.c_str()))
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
        }
        if (count == 0)
        {
            hasError  = true;
            error_msg = "Missing parameters";
        }
    }
#endif  // ESP3D_BT_FEATURE

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