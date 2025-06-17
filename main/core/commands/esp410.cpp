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
#include <stdio.h>

#include <cstring>
#include <string>

#include "authentication/esp3d_authentication.h"
#include "esp3d_client.h"
#include "esp3d_commands.h"
#include "esp3d_string.h"
#include "esp3d_version.h"
#include "filesystem/esp3d_flash.h"
#include "network/esp3d_network.h"

#define COMMAND_ID         410
#define MAX_SCAN_LIST_SIZE 15
// Get available AP list
// output is JSON or plain text according parameter
//[ESP410]<WIFI/BTSERIAL/BTBLE>json=<no>
void ESP3DCommands::ESP410(int cmd_params_pos, ESP3DMessage *msg)
{
    ESP3DClientType target = msg->origin;
    ESP3DRequest requestId = msg->request_id;
    msg->target            = target;
    msg->origin            = ESP3DClientType::command;
    bool json              = hasTag(msg, cmd_params_pos, "json");
    std::string tmpstr;
#if ESP3D_WIFI_FEATURE
    bool scanWiFi = hasTag(msg, cmd_params_pos, "WIFI");
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_BT_FEATURE
    bool scanBTSerial = hasTag(msg, cmd_params_pos, "BTSERIAL");
    bool scanBTBLE    = hasTag(msg, cmd_params_pos, "BTBLE");
#endif  // ESP3D_BT_FEATURE

    uint8_t count = 0;
#if ESP3D_WIFI_FEATURE
    count += scanWiFi ? 1 : 0;
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_BT_FEATURE
    count += scanBTSerial ? 1 : 0;
    count += scanBTBLE ? 1 : 0;
#endif  // ESP3D_BT_FEATURE
#if ESP3D_WIFI_FEATURE && !ESP3D_BT_FEATURE
    if (count == 0)
    {
        scanWiFi = true;  // default to WiFi scan
        count++;
    }
#endif  // ESP3D_WIFI_FEATURE && ! ESP3D_BT_FEATURE
#if ESP3D_BT_FEATURE
    if (count == 0)
    {
        tmpstr = "Invalid parameters";
        dispatchAnswer(msg, COMMAND_ID, json, true, tmpstr.c_str());
        return;
    }
#endif  // ESP3D_BT_FEATURE

    if (count > 1)
    {
        tmpstr = "Multiple scan types not allowed";
        dispatchAnswer(msg, COMMAND_ID, json, true, tmpstr.c_str());
        return;
    }
#if ESP3D_AUTHENTICATION_FEATURE
    if (msg->authentication_level == ESP3DAuthenticationLevel::guest)
    {
        dispatchAuthenticationError(msg, COMMAND_ID, json);
        return;
    }
#endif  // ESP3D_AUTHENTICATION_FEATURE
    ESP3DRadioMode radio_mode = esp3dNetwork.getMode();
    if (radio_mode == ESP3DRadioMode::none)
    {
        tmpstr = "Radio not enabled";
        dispatchAnswer(msg, COMMAND_ID, json, true, tmpstr.c_str());
        return;
    }
#if ESP3D_WIFI_FEATURE
    if (scanWiFi
        && !(radio_mode == ESP3DRadioMode::wifi_sta || radio_mode == ESP3DRadioMode::wifi_ap
             || radio_mode == ESP3DRadioMode::wifi_ap_config
             || radio_mode == ESP3DRadioMode::wifi_ap_limited))
    {
        tmpstr = "WiFi scan not available in current mode";
        dispatchAnswer(msg, COMMAND_ID, json, true, tmpstr.c_str());
        return;
    }
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_BT_FEATURE
    if (scanBTSerial && radio_mode != ESP3DRadioMode::bluetooth_serial)
    {
        tmpstr = "Bluetooth Serial scan not available in current mode";
        dispatchAnswer(msg, COMMAND_ID, json, true, tmpstr.c_str());
        return;
    }
    if (scanBTBLE && radio_mode != ESP3DRadioMode::bluetooth_ble)
    {
        tmpstr = "Bluetooth BLE scan not available in current mode";
        dispatchAnswer(msg, COMMAND_ID, json, true, tmpstr.c_str());
        return;
    }
#endif  // ESP3D_BT_FEATURE

    if (json)
    {
        tmpstr = "{\"cmd\":\"410\",\"status\":\"ok\",\"data\":[";
    }
    else
    {
        tmpstr = "Start Scan\n";
    }
    msg->type = ESP3DMessageType::head;
    if (!dispatch(msg, tmpstr.c_str()))
    {
        esp3d_log_e("Error sending response to clients");
        return;
    }
#if ESP3D_WIFI_FEATURE

    if (scanWiFi)
    {
        // WiFi scan
        if (radio_mode != ESP3DRadioMode::wifi_sta)
        {
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        }
        esp_err_t res = esp_wifi_scan_start(NULL, true);

        if (res == ESP_OK)
        {
            uint16_t number = MAX_SCAN_LIST_SIZE;
            wifi_ap_record_t ap_info[MAX_SCAN_LIST_SIZE];
            uint16_t ap_count = 0;
            memset(ap_info, 0, sizeof(ap_info));
            uint16_t real_count = 0;
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
            esp3d_log_d("Total : %d / %d (max : %d)", ap_count, number, MAX_SCAN_LIST_SIZE);

            for (int i = 0; (i < MAX_SCAN_LIST_SIZE) && (i < ap_count); i++)
            {
                esp3d_log_d("%s %d %d", ap_info[i].ssid, ap_info[i].rssi, ap_info[i].authmode);
                tmpstr         = "";
                int32_t signal = esp3dNetwork.getSignal(ap_info[i].rssi, false);
                if (signal != 0)
                {
                    if (json)
                    {
                        if (real_count > 0)
                        {
                            tmpstr += ",";
                        }
                        real_count++;
                        tmpstr += "{\"SSID\":\"";
                    }
                    tmpstr += (char *)(ap_info[i].ssid);
                    if (json)
                    {
                        tmpstr += "\",\"SIGNAL\":\"";
                    }
                    else
                    {
                        tmpstr += "\t";
                    }
                    tmpstr += std::to_string(signal);
                    if (json)
                    {
                        tmpstr += "\",\"IS_PROTECTED\":\"";
                        if (ap_info[i].authmode != WIFI_AUTH_OPEN)
                        {
                            tmpstr += "1\"}";
                        }
                        else
                        {
                            tmpstr += "0\"}";
                        }
                    }
                    else
                    {
                        tmpstr += "%\t";
                        if (ap_info[i].authmode != WIFI_AUTH_OPEN)
                        {
                            tmpstr += "Secure\n";
                        }
                        else
                        {
                            tmpstr += "Open\n";
                        }
                    }
                    if (!dispatch(tmpstr.c_str(), target, requestId, ESP3DMessageType::core))
                    {
                        esp3d_log_e("Error sending answer to clients");
                    }
                }
            }
            esp_wifi_clear_ap_list();
        }

        if (esp3dNetwork.getMode() != ESP3DRadioMode::wifi_sta)
        {
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        }
    }
    else
    {
    }
}
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_BT_FEATURE
if (scanBTSerial)
{
    // Bluetooth Serial scan
    if (!btSerialClient.started())
    {
        esp3d_log_e("Bluetooth Serial client not started");
        if (json)
        {
            tmpstr = "]}";
        }
        else
        {
            tmpstr = "Bluetooth Serial not started\n";
        }
        dispatch(tmpstr.c_str(), target, requestId, ESP3DMessageType::tail);
        return;
    }

    std::vector<BTDevice> devices;
    if (btSerialClient.scan(devices))
    {
        uint16_t real_count = 0;
        for (size_t i = 0; i < devices.size() && i < MAX_SCAN_LIST_SIZE; i++)
        {
            if (!devices[i].name.empty())
            {  // Ignorer les appareils sans nom
                tmpstr = "";
                if (json)
                {
                    if (real_count > 0)
                    {
                        tmpstr += ",";
                    }
                    real_count++;
                    tmpstr += "{\"NAME\":\"";
                }
                tmpstr += devices[i].name;
                if (json)
                {
                    tmpstr += "\",\"ADDRESS\":\"";
                }
                else
                {
                    tmpstr += " ";
                }
                char addr_str[18];
                btSerialClient.bda2str(devices[i].addr, addr_str, sizeof(addr_str));
                tmpstr += addr_str;
                if (json)
                {
                    tmpstr += "\"}";
                }
                else
                {
                    tmpstr += "\n";
                }
                if (!dispatch(tmpstr.c_str(), target, requestId, ESP3DMessageType::core))
                {
                    esp3d_log_e("Error sending answer to clients");
                }
            }
        }
        esp3d_log_d("Total Bluetooth devices: %d (max: %d)", devices.size(), MAX_SCAN_LIST_SIZE);
    }
    else
    {
        esp3d_log_e("Bluetooth Serial scan failed");
        if (json)
        {
            tmpstr = "]}";
        }
        else
        {
            tmpstr = "Scan failed\n";
        }
        dispatch(tmpstr.c_str(), target, requestId, ESP3DMessageType::tail);
        return;
    }
}
if (scanBTBLE)
{
    // TODO: Implement Bluetooth BLE scan
}
#endif
// end of list
if (json)
{
    tmpstr = "]}";
}
else
{
    tmpstr = "End Scan\n";
}
if (!dispatch(tmpstr.c_str(), target, requestId, ESP3DMessageType::tail))
{
    esp3d_log_e("Error sending answer to clients");
}
}
#endif  // ESP3D_WIFI_FEATURE || ESP3D_BT_FEATURE