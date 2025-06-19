
/*
  esp3d_settings.cpp -  settings esp3d functions class

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

#include "esp3d_target_settings.h"
#include "nvs_flash.h"
#include "nvs_handle.hpp"

#if ESP3D_WIFI_FEATURE
#include "lwip/ip_addr.h"
#endif  // ESP3D_WIFI_FEATURE
#include <cstring>
#include <regex>
#include <string>

#include "board_config.h"
#include "esp3d_client.h"
#include "esp3d_log.h"
#include "esp_system.h"

#if ESP3D_USB_SERIAL_FEATURE
#include "usb_serial_def.h"
#endif  // ESP3D_USB_SERIAL_FEATURE
#include "network/esp3d_network.h"
#if ESP3D_NOTIFICATIONS_FEATURE
#include "notifications/esp3d_notifications_service.h"
#endif  // ESP3D_NOTIFICATIONS_FEATURE

#include "authentication/esp3d_authentication.h"
#include "board_init.h"

#define STORAGE_NAME    "ESP3D_TFT"
#define SETTING_VERSION "ESP3D_TFT-V1.0.0"

#if ESP3D_TFT_LOG
static const char *esp3d_setting_names[] = {"esp3d_version",
                                            "esp3d_baud_rate",
                                            "esp3d_spi_divider",
                                            "esp3d_radio_boot_mode",
                                            "esp3d_radio_mode",
                                            "esp3d_fallback_mode",
#if ESP3D_WIFI_FEATURE
                                            "esp3d_sta_ssid",
                                            "esp3d_sta_password",
                                            "esp3d_sta_ip_mode",
                                            "esp3d_sta_ip_static",
                                            "esp3d_sta_mask_static",
                                            "esp3d_sta_gw_static",
                                            "esp3d_sta_dns_static",
                                            "esp3d_ap_ssid",
                                            "esp3d_ap_password",
                                            "esp3d_ap_ip_static",
                                            "esp3d_ap_channel",
#endif  // ESP3D_WIFI_FEATURE
                                            "esp3d_hostname",
#if ESP3D_WIFI_FEATURE
                                            "esp3d_http_port",
                                            "esp3d_http_on",
#endif  // ESP3D_WIFI_FEATURE
                                            "esp3d_setup",
                                            "esp3d_target_firmware",
#if ESP3D_SD_CARD_FEATURE
                                            "esp3d_check_update_on_sd",
#endif  // ESP3D_SD_CARD_FEATURE
#if ESP3D_NOTIFICATIONS_FEATURE
                                            "esp3d_notification_type",
                                            "esp3d_auto_notification",
                                            "esp3d_notification_token_1",
                                            "esp3d_notification_token_2",
                                            "esp3d_notification_token_setting",
#endif  // ESP3D_NOTIFICATIONS_FEATURE
#if ESP3D_WIFI_FEATURE
#if ESP3D_TELNET_FEATURE
                                            "esp3d_socket_port",
                                            "esp3d_socket_on",
#endif  // ESP3D_TELNET_FEATURE
#if ESP3D_WEBSOCKET_FEATURE
                                            "esp3d_ws_on",
#endif  // ESP3D_WEBSOCKET_FEATURE
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_USB_SERIAL_FEATURE
                                            "esp3d_usb_serial_baud_rate",
#endif  // #if ESP3D_USB_SERIAL_FEATURE
                                            "esp3d_output_client",
#if ESP3D_AUTHENTICATION_FEATURE
                                            "esp3d_admin_password",
                                            "esp3d_user_password",
                                            "esp3d_session_timeout",
#endif  // ESP3D_AUTHENTICATION_FEATURE
                                            "esp3d_ui_language",
                                            "esp3d_jog_type",
                                            "esp3d_polling_on",
                                            "esp3d_inverved_x",
                                            "esp3d_inverved_y",
                                            "esp3d_workspace_width",
                                            "esp3d_workspace_depth",
                                            "esp3d_extensions",
                                            "esp3d_pause_script",
                                            "esp3d_stop_script",
                                            "esp3d_resume_script",
#if ESP3D_TIMESTAMP_FEATURE
                                            "esp3d_use_internet_time",
                                            "esp3d_time_server1",
                                            "esp3d_time_server2",
                                            "esp3d_time_server3",
                                            "esp3d_timezone",
#endif  // ESP3D_TIMESTAMP_FEATURE
#if ESP3D_WEBDAV_SERVICES_FEATURE
                                            "esp3d_webdav_on",
#endif  // ESP3D_WEBDAV_SERVICES_FEATURE
#if ESP3D_BT_FEATURE
                                            "esp3d_btserial_address",
                                            "esp3d_btserial_pin",
                                            "esp3d_btble_id",
                                            "esp3d_btble_passkey",
#endif  // ESP3D_BT_FEATURE
#if ESP3D_BUZZER_FEATURE
                                            "esp3d_buzzer_on",
#endif  // ESP3D_BUZZER_FEATURE
#if ESP3D_DISPLAY_FEATURE
                                            "esp3d_brightness_level",
#endif
                                            // Note: esp3d_target_settings_list.inc devra être géré
                                            // séparément
                                            "unknown_index"};

// Fonction utilitaire pour obtenir le nom d'un setting
inline const char *get_setting_name(ESP3DSettingIndex index)
{
    uint16_t idx = static_cast<uint16_t>(index);
    if (idx < sizeof(esp3d_setting_names) / sizeof(esp3d_setting_names[0]))
    {
        return esp3d_setting_names[idx];
    }
    return "invalid_index";
}
#endif  // ESP3D_TFT_LOG

ESP3DSettings esp3dTftsettings;

const uint8_t SupportedBaudListSize = sizeof(SupportedBaudList) / sizeof(uint32_t);

const uint8_t SupportedSPIDivider[]   = {1, 2, 4, 6, 8, 16, 32};
const uint8_t SupportedSPIDividerSize = sizeof(SupportedSPIDivider) / sizeof(uint8_t);

const uint8_t SupportedApChannels[]   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
const uint8_t SupportedApChannelsSize = sizeof(SupportedApChannels) / sizeof(uint8_t);
#if ESP3D_TIMESTAMP_FEATURE
const char *SupportedTimeZones[] = {"-12:00", "-11:00", "-10:00", "-09:00", "-08:00", "-07:00",
                                    "-06:00", "-05:00", "-04:00", "-03:30", "-03:00", "-02:00",
                                    "-01:00", "+00:00", "+01:00", "+02:00", "+03:00", "+03:30",
                                    "+04:00", "+04:30", "+05:00", "+05:30", "+05:45", "+06:00",
                                    "+06:30", "+07:00", "+08:00", "+08:45", "+09:00", "+09:30",
                                    "+10:00", "+10:30", "+11:00", "+12:00", "+12:45", "+13:00",
                                    "+14:00"};

const uint8_t SupportedTimeZonesSize = sizeof(SupportedTimeZones) / sizeof(const char *);
#endif  // ESP3D_TIMESTAMP_FEATURE

// value of settings, default value are all strings
const ESP3DSettingDescription ESP3DSettingsData[] = {
    {ESP3DSettingIndex::esp3d_version,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_VERSION,
     "Invalid data"},  // Version
    {ESP3DSettingIndex::esp3d_baud_rate,
     ESP3DSettingType::integer_t,
     4,
     UART_BAUD_RATE_STR},  // BaudRate
    {ESP3DSettingIndex::esp3d_ui_language,
     ESP3DSettingType::string_t,
     SIZE_OF_UI_LANGUAGE,
     "default"},  // Language

// Include target specific settings
#include "esp3d_target_settings_values.inc"  // Target values

#if ESP3D_USB_SERIAL_FEATURE
    {ESP3DSettingIndex::esp3d_usb_serial_baud_rate,
     ESP3DSettingType::integer_t,
     4,
     ESP3D_USB_SERIAL_BAUDRATE},  // BaudRate
#endif                            // ESP3D_USB_SERIAL_FEATURE
    {ESP3DSettingIndex::esp3d_output_client, ESP3DSettingType::byte_t, 1, "1"},

    {ESP3DSettingIndex::esp3d_hostname,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_HOSTNAME,
     "esp3d-tft"},
    {ESP3DSettingIndex::esp3d_radio_boot_mode, ESP3DSettingType::byte_t, 1, "1"},
    {ESP3DSettingIndex::esp3d_radio_mode, ESP3DSettingType::byte_t, 1, "3"},
#if ESP3D_BT_FEATURE
    {ESP3DSettingIndex::esp3d_btserial_address,
     ESP3DSettingType::string_t,
     SIZE_OF_BT_SERIAL_ADDRESS,
     BT_ADDRESS_TARGET},
    {ESP3DSettingIndex::esp3d_btserial_pin,
     ESP3DSettingType::string_t,
     SIZE_OF_BT_SERIAL_PIN,
     BTSERIAL_PIN_TARGET},
    {ESP3DSettingIndex::esp3d_btble_id,
     ESP3DSettingType::string_t,
     SIZE_OF_BT_BLE_ID,
     BT_ADDRESS_TARGET},
    {ESP3DSettingIndex::esp3d_btble_passkey,
     ESP3DSettingType::string_t,
     SIZE_OF_BT_BLE_PASSKEY,
     BTBLE_PIN_TARGET},
#endif  // ESP3D_BT_FEATURE
#if ESP3D_WIFI_FEATURE
    {ESP3DSettingIndex::esp3d_fallback_mode, ESP3DSettingType::byte_t, 1, "3"},
    {ESP3DSettingIndex::esp3d_sta_ssid, ESP3DSettingType::string_t, SIZE_OF_SETTING_SSID_ID, ""},
    {ESP3DSettingIndex::esp3d_sta_password,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_SSID_PWD,
     ""},
    {ESP3DSettingIndex::esp3d_sta_ip_mode, ESP3DSettingType::byte_t, 1, "0"},
    {ESP3DSettingIndex::esp3d_sta_ip_static, ESP3DSettingType::ip_t, 4, "192.168.1.100"},
    {ESP3DSettingIndex::esp3d_sta_mask_static, ESP3DSettingType::ip_t, 4, "255.255.255.0"},
    {ESP3DSettingIndex::esp3d_sta_gw_static, ESP3DSettingType::ip_t, 4, "192.168.1.1"},
    {ESP3DSettingIndex::esp3d_sta_dns_static, ESP3DSettingType::ip_t, 4, "192.168.1.1"},
    {ESP3DSettingIndex::esp3d_ap_ssid,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_SSID_ID,
     "esp3dtft"},
    {ESP3DSettingIndex::esp3d_ap_password,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_SSID_PWD,
     "12345678"},
    {ESP3DSettingIndex::esp3d_ap_ip_static, ESP3DSettingType::ip_t, 4, "192.168.0.1"},
    {ESP3DSettingIndex::esp3d_ap_channel, ESP3DSettingType::byte_t, 1, "2"},
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_HTTP_FEATURE
    {ESP3DSettingIndex::esp3d_http_port, ESP3DSettingType::integer_t, 4, "80"},
    {ESP3DSettingIndex::esp3d_http_on, ESP3DSettingType::byte_t, 1, "1"},
#endif  // ESP3D_HTTP_FEATURE
    {ESP3DSettingIndex::esp3d_setup, ESP3DSettingType::byte_t, 1, "0"},
    {ESP3DSettingIndex::esp3d_target_firmware, ESP3DSettingType::byte_t, 1, "0"},
#if ESP3D_SD_CARD_FEATURE
#if SD_INTERFACE_TYPE == 0
    {ESP3DSettingIndex::esp3d_spi_divider, ESP3DSettingType::byte_t, 1, "1"},  // SPIdivider
#endif  // SD_INTERFACE_TYPE == 0
#if ESP3D_UPDATE_FEATURE
    {ESP3DSettingIndex::esp3d_check_update_on_sd, ESP3DSettingType::byte_t, 1, "1"},
#endif  // ESP3D_UPDATE_FEATURE
#endif  // ESP3D_SD_CARD_FEATURE
#if ESP3D_NOTIFICATIONS_FEATURE
    {ESP3DSettingIndex::esp3d_notification_type, ESP3DSettingType::byte_t, 1, "0"},
    {ESP3DSettingIndex::esp3d_auto_notification, ESP3DSettingType::byte_t, 1, "0"},
    {ESP3DSettingIndex::esp3d_notification_token_1,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_NOFIFICATION_T1,
     ""},
    {ESP3DSettingIndex::esp3d_notification_token_2,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_NOFIFICATION_T2,
     ""},
    {ESP3DSettingIndex::esp3d_notification_token_setting,
     ESP3DSettingType::string_t,
     SIZE_OF_SETTING_NOFIFICATION_TS,
     ""},
#endif  // ESP3D_NOTIFICATIONS_FEATURE
#if ESP3D_TELNET_FEATURE
    {ESP3DSettingIndex::esp3d_socket_port, ESP3DSettingType::integer_t, 4, "23"},
    {ESP3DSettingIndex::esp3d_socket_on, ESP3DSettingType::byte_t, 1, "1"},
#endif  // ESP3D_TELNET_FEATURE
#if ESP3D_WS_SERVICE_FEATURE
    {ESP3DSettingIndex::esp3d_ws_on, ESP3DSettingType::byte_t, 1, "1"},
#endif  // ESP3D_WS_SERVICE_FEATURE
#if ESP3D_WEBDAV_SERVICES_FEATURE
    {ESP3DSettingIndex::esp3d_webdav_on, ESP3DSettingType::byte_t, 1, "1"},
#endif  // ESP3D_WEBDAV_SERVICES_FEATURE
#if ESP3D_AUTHENTICATION_FEATURE
    {ESP3DSettingIndex::esp3d_admin_password,
     ESP3DSettingType::string_t,
     SIZE_OF_LOCAL_PASSWORD,
     "admin"},
    {ESP3DSettingIndex::esp3d_user_password,
     ESP3DSettingType::string_t,
     SIZE_OF_LOCAL_PASSWORD,
     "user"},
    {ESP3DSettingIndex::esp3d_session_timeout, ESP3DSettingType::byte_t, 1, "3"},
#endif  // ESP3D_AUTHENTICATION_FEATURE
#if ESP3D_DISPLAY_FEATURE
    {ESP3DSettingIndex::esp3d_jog_type, ESP3DSettingType::byte_t, 1, "0"},
    {ESP3DSettingIndex::esp3d_polling_on, ESP3DSettingType::byte_t, 1, "1"},
    {ESP3DSettingIndex::esp3d_inverved_x, ESP3DSettingType::byte_t, 1, "0"},
    {ESP3DSettingIndex::esp3d_inverved_y, ESP3DSettingType::byte_t, 1, "0"},
    {ESP3DSettingIndex::esp3d_workspace_width, ESP3DSettingType::float_t, 3, "100.00"},
    {ESP3DSettingIndex::esp3d_workspace_depth, ESP3DSettingType::float_t, 3, "100.00"},
#if ESP3D_BRIGHTNESS_CONTROL_FEATURE
    {ESP3DSettingIndex::esp3d_brightness_level, ESP3DSettingType::byte_t, 1, "100"},
#endif  // ESP3D_BRIGHTNESS_CONTROL_FEATURE
#endif  // ESP3D_DISPLAY_FEATURE
    {ESP3DSettingIndex::esp3d_stop_script, ESP3DSettingType::string_t, SIZE_OF_SCRIPT, ""},
    {ESP3DSettingIndex::esp3d_pause_script, ESP3DSettingType::string_t, SIZE_OF_SCRIPT, ""},
    {ESP3DSettingIndex::esp3d_resume_script, ESP3DSettingType::string_t, SIZE_OF_SCRIPT, ""},
#if ESP3D_TIMESTAMP_FEATURE
    {ESP3DSettingIndex::esp3d_use_internet_time, ESP3DSettingType::byte_t, 1, "1"},
    {ESP3DSettingIndex::esp3d_time_server1,
     ESP3DSettingType::string_t,
     SIZE_OF_SERVER_URL,
     "0.pool.ntp.org"},
    {ESP3DSettingIndex::esp3d_time_server2,
     ESP3DSettingType::string_t,
     SIZE_OF_SERVER_URL,
     "1.pool.ntp.org"},
    {ESP3DSettingIndex::esp3d_time_server3,
     ESP3DSettingType::string_t,
     SIZE_OF_SERVER_URL,
     "2.pool.ntp.org"},
    {ESP3DSettingIndex::esp3d_timezone, ESP3DSettingType::string_t, SIZE_OF_TIMEZONE, "+00:00"},
#endif  // ESP3D_TIMESTAMP_FEATURE
#if ESP3D_BUZZER_FEATURE
    {ESP3DSettingIndex::esp3d_buzzer_on, ESP3DSettingType::byte_t, 1, "1"},
#endif  // ESP3D_BUZZER_FEATURE

};

// Is Valid String Setting ?
bool ESP3DSettings::isValidStringSetting(const char *value, ESP3DSettingIndex settingElement)
{
    const ESP3DSettingDescription *settingPtr = getSettingPtr(settingElement);
    if (!settingPtr)
    {
        return false;
    }
    if (settingPtr->type == ESP3DSettingType::float_t)
    {
        // check if it is a float stored as string
        if (strlen(value) > 15 || strlen(value) == 0)
        {
            return false;
        }
        switch (settingElement)
        {
            case ESP3DSettingIndex::esp3d_workspace_width:
            case ESP3DSettingIndex::esp3d_workspace_depth:
                if (strtod(value, NULL) == 0)
                {
                    return false;
                }
                break;
            default:
                break;
        }
        // check if any alpha char not supposed to be here
        for (uint8_t i = 0; i < strlen(value); i++)
        {
            if (value[i] == '.' || value[i] == ',' || value[i] == ' ' || value[i] == '-'
                || value[i] == '+' || value[i] == 'e' || value[i] == 'E'
                || (value[i] >= '0' && value[i] <= '9'))
            {
                continue;
            }
            else
            {
                return false;
            }
        }
        return true;
    }
    if (!(settingPtr->type == ESP3DSettingType::string_t))
    {
        return false;
    }
    // use strlen because it crash with regex if value is longer than 41
    // characters
    size_t len = strlen(value);

    if (len > settingPtr->size)
    {
        return false;
    }
    // check if any non printable char not supposed to be here
    for (uint i = 0; i < strlen(value); i++)
    {
        if (!std::isprint(value[i]))
        {
            return false;
        }
    }
    switch (settingElement)
    {
#if ESP3D_TIMESTAMP_FEATURE
        case ESP3DSettingIndex::esp3d_timezone:
            if (len != settingPtr->size - 1)
            {
                return false;
            }
            for (uint8_t i = 0; i < SupportedTimeZonesSize; i++)
            {
                if (strcmp(value, SupportedTimeZones[i]) == 0)
                {
                    return true;
                }
            }
            break;
#endif  // ESP3D_TIMESTAMP_FEATURE
        case ESP3DSettingIndex::esp3d_version:
            return strcmp(value, SETTING_VERSION) == 0;
            break;
        case ESP3DSettingIndex::esp3d_pause_script:
        case ESP3DSettingIndex::esp3d_resume_script:
        case ESP3DSettingIndex::esp3d_stop_script:
#if ESP3D_TIMESTAMP_FEATURE
        case ESP3DSettingIndex::esp3d_time_server1:
        case ESP3DSettingIndex::esp3d_time_server2:
        case ESP3DSettingIndex::esp3d_time_server3:
#endif                    // ESP3D_TIMESTAMP_FEATURE
            return true;  // len test already done so return true
#if ESP3D_BT_FEATURE
        case ESP3DSettingIndex::esp3d_btserial_address:
            esp3d_log("Checking address validity");
            // length must be 17 characters
            // 12 characters for MAC address + 5 colons
            if (strlen(value)!= 17)
            {
                esp3d_log_e("Invalid address length %d,  expected 17", len);
                return false;
            }
            if (strncmp(value, "00:00:00:00:00:00", 17) == 0)
            {  // check if it is not all zeros
                esp3d_log_e("Invalid address, all zeros");
                return false;
            }
            // parse the address
            for (size_t i = 0; i < 17; i++)
            {
                if (i % 3 == 2)
                {  // every 3rd character must be a colon
                    // check if it is a colon
                    if (value[i] != ':')
                    {
                        esp3d_log_e("Invalid address, colon not found at position %d", i);
                        return false;
                    }
                }
                else
                {  // every other character must be a hex digit
                    // check if it is a hex digit
                    if (!std::isxdigit(value[i]))
                    {
                        esp3d_log_e("Invalid address, hex digit not found at position %d", i);
                        return false;
                    }
                }
            }
            esp3d_log("Address is valid");
            return true;
        case ESP3DSettingIndex::esp3d_btble_id:
            return (len > 0 && len <= SIZE_OF_BT_BLE_ID);  // any string from 1 to 32
        case ESP3DSettingIndex::esp3d_btserial_pin:
            return (len == 0 || len == SIZE_OF_BT_SERIAL_PIN);  // any string of size 4 or 0

        case ESP3DSettingIndex::esp3d_btble_passkey:
            return (len <= SIZE_OF_BT_SERIAL_PIN);  // any string from 0 to 16
#endif                                              // ESP3D_BT_FEATURE
#if ESP3D_WIFI_FEATURE
        case ESP3DSettingIndex::esp3d_ap_ssid:
        case ESP3DSettingIndex::esp3d_sta_ssid:
            return (len > 0 && len <= SIZE_OF_SETTING_SSID_ID);  // any string from 1 to 32
        case ESP3DSettingIndex::esp3d_sta_password:
        case ESP3DSettingIndex::esp3d_ap_password:
            return (
                len == 0
                || (len >= 8 && len <= SIZE_OF_SETTING_SSID_PWD));  // any string from 8 to 64 or 0
#endif                                                              // ESP3D_WIFI_FEATURE
        case ESP3DSettingIndex::esp3d_ui_language:
            return (len <= SIZE_OF_UI_LANGUAGE && len > 0);
        case ESP3DSettingIndex::esp3d_hostname:
            esp3d_log("Checking hostname validity");
            return std::regex_match(value,
                                    std::regex(
                                        "^[a-zA-Z0-9]{1}[a-zA-Z0-9\\-]{0,31}$"));  // any string
                                                                                   // alphanumeric
                                                                                   // or '-' from 1
                                                                                   // to 32
#if ESP3D_NOTIFICATIONS_FEATURE
        case ESP3DSettingIndex::esp3d_notification_token_1:
            return len <= SIZE_OF_SETTING_NOFIFICATION_T1;  // any string from 0 to 64
        case ESP3DSettingIndex::esp3d_notification_token_2:
            return len <= SIZE_OF_SETTING_NOFIFICATION_T2;  // any string from 0 to 64
        case ESP3DSettingIndex::esp3d_notification_token_setting:
            return len <= SIZE_OF_SETTING_NOFIFICATION_TS;  // any string from 0 to
                                                            // 128
#endif                                                      // ESP3D_NOTIFICATIONS_FEATURE
#if ESP3D_AUTHENTICATION_FEATURE
        case ESP3DSettingIndex::esp3d_admin_password:
        case ESP3DSettingIndex::esp3d_user_password:
            for (uint i = 0; i < strlen(value); i++)
            {
                if (value[i] == ' ')
                {  // no space allowed
                    return false;
                }
            }
            return len <= SIZE_OF_LOCAL_PASSWORD;  // any string from 0 to 20
#endif                                             // ESP3D_AUTHENTICATION_FEATURE
        default:
            return isValidStringTargetSetting(value, settingElement);
    }
    return false;
}

// Is Valid Integer Setting ?
bool ESP3DSettings::isValidIntegerSetting(uint32_t value, ESP3DSettingIndex settingElement)
{
    const ESP3DSettingDescription *settingPtr = getSettingPtr(settingElement);
    if (!settingPtr)
    {
        return false;
    }
    if (!(settingPtr->type == ESP3DSettingType::integer_t
          || settingPtr->type == ESP3DSettingType::ip_t))
    {
        return false;
    }
    switch (settingElement)
    {
#if ESP3D_USB_SERIAL_FEATURE
        case ESP3DSettingIndex::esp3d_usb_serial_baud_rate:
#endif  // #if ESP3D_USB_SERIAL_FEATURE
        case ESP3DSettingIndex::esp3d_baud_rate:
            for (uint8_t i = 0; i < SupportedBaudListSize; i++)
            {
                if (SupportedBaudList[i] == value)
                {
                    return true;
                }
            }
            break;
#if ESP3D_TELNET_FEATURE
        case ESP3DSettingIndex::esp3d_socket_port:
#endif  // ESP3D_TELNET_FEATURE
#if ESP3D_HTTP_FEATURE
        case ESP3DSettingIndex::esp3d_http_port:
#endif  // ESP3D_HTTP_FEATURE
#if ESP3D_HTTP_FEATURE || ESP3D_TELNET_FEATURE
            if (value >= 1 && value < 65535)
            {
                return true;
            }
            break;
#endif  // ESP3D_HTTP_FEATURE  || ESP3D_TELNET_FEATURE

        default:
            return isValidIntegerTargetSetting(value, settingElement);
    }
    return false;
}

// Is Valid Byte Setting ?
bool ESP3DSettings::isValidByteSetting(uint8_t value, ESP3DSettingIndex settingElement)
{
    const ESP3DSettingDescription *settingPtr = getSettingPtr(settingElement);
    if (!settingPtr)
    {
        return false;
    }
    if (settingPtr->type != ESP3DSettingType::byte_t)
    {
        return false;
    }
    switch (settingElement)
    {
#if ESP3D_SD_CARD_FEATURE
#if ESP3D_UPDATE_FEATURE
        case ESP3DSettingIndex::esp3d_check_update_on_sd:
#endif  // ESP3D_UPDATE_FEATURE
#endif  // ESP3D_SD_CARD_FEATURE
#if ESP3D_DISPLAY_FEATURE
        case ESP3DSettingIndex::esp3d_inverved_x:
        case ESP3DSettingIndex::esp3d_inverved_y:
        case ESP3DSettingIndex::esp3d_jog_type:
        case ESP3DSettingIndex::esp3d_polling_on:
#endif  // ESP3D_DISPLAY_FEATURE
        case ESP3DSettingIndex::esp3d_setup:
#if ESP3D_TELNET_FEATURE
        case ESP3DSettingIndex::esp3d_socket_on:
#endif  // ESP3D_TELNET_FEATURE
#if ESP3D_WS_SERVICE_FEATURE
        case ESP3DSettingIndex::esp3d_ws_on:
#endif  // ESP3D_WS_SERVICE_FEATURE
#if ESP3D_HTTP_FEATURE
        case ESP3DSettingIndex::esp3d_http_on:
#if ESP3D_WEBDAV_SERVICES_FEATURE
        case ESP3DSettingIndex::esp3d_webdav_on:
#endif  // ESP3D_WEBDAV_SERVICES_FEATURE
#endif  // ESP3D_HTTP_FEATURE
        case ESP3DSettingIndex::esp3d_radio_boot_mode:
#if ESP3D_NOTIFICATIONS_FEATURE
        case ESP3DSettingIndex::esp3d_auto_notification:
#endif  // ESP3D_NOTIFICATIONS_FEATURE
#if ESP3D_TIMESTAMP_FEATURE
        case ESP3DSettingIndex::esp3d_use_internet_time:
#endif  // ESP3D_TIMESTAMP_FEATURE
#if ESP3D_BUZZER_FEATURE
        case ESP3DSettingIndex::esp3d_buzzer_on:
#endif  // ESP3D_BUZZER_FEATURE
            if (value == (uint8_t)ESP3DState::off || value == (uint8_t)ESP3DState::on)
            {
                return true;
            }
            break;
#if ESP3D_BRIGHTNESS_CONTROL_FEATURE
        case ESP3DSettingIndex::esp3d_brightness_level:
            // brightness level is 0-100
            if (value <= 100)
            {
                return true;
            }
            break;
#endif  // ESP3D_BRIGHTNESS_CONTROL_FEATURE
#if ESP3D_AUTHENTICATION_FEATURE
        case ESP3DSettingIndex::esp3d_session_timeout:
            return true;  // 0 ->255 minutes
            break;
#endif  // ESP3D_AUTHENTICATION_FEATURE
        case ESP3DSettingIndex::esp3d_output_client:
            esp3d_log("esp3d_output_client value %d", value);

            if (value == (uint8_t)ESP3DClientType::serial
#if ESP3D_USB_SERIAL_FEATURE
                || value == (uint8_t)ESP3DClientType::usb_serial
#endif  // ESP3D_USB_SERIAL_FEATURE
#if ESP3D_BT_FEATURE
                || value == (uint8_t)ESP3DClientType::bt_serial
                || value == (uint8_t)ESP3DClientType::bt_ble
#endif  // ESP3D_BT_FEATURE
            )
            {
                return true;  // serial, usb_serial, bt_serial, bt_ble
            }
            else
            {
                // other client types are not supported
                esp3d_log("esp3d_output_client value %d %d not supported",
                          value,
                          static_cast<uint8_t>(ESP3DClientType::serial));
                return false;
            }

            break;
#if ESP3D_NOTIFICATIONS_FEATURE
        case ESP3DSettingIndex::esp3d_notification_type:
            if (value == static_cast<uint8_t>(ESP3DNotificationType::none)
                || value == static_cast<uint8_t>(ESP3DNotificationType::pushover)
                || value == static_cast<uint8_t>(ESP3DNotificationType::email)
                || value == static_cast<uint8_t>(ESP3DNotificationType::line)
                || value == static_cast<uint8_t>(ESP3DNotificationType::telegram)
                || value == static_cast<uint8_t>(ESP3DNotificationType::ifttt))
            {
                return true;
            }
            break;
#endif  // ESP3D_NOTIFICATIONS_FEATURE
#if ESP3D_WIFI_FEATURE
        case ESP3DSettingIndex::esp3d_sta_ip_mode:
            if (value == static_cast<uint8_t>(ESP3DIpMode::dhcp)
                || value == static_cast<uint8_t>(ESP3DIpMode::staticIp))
            {
                return true;
            }
            break;
        case ESP3DSettingIndex::esp3d_fallback_mode:
            if (value == (uint8_t)ESP3DRadioMode::off
                || value == (uint8_t)ESP3DRadioMode::wifi_ap_config
                || value == (uint8_t)ESP3DRadioMode::bluetooth_serial)
            {
                return true;
            }
            break;
#endif  // ESP3D_WIFI_FEATURE
        case ESP3DSettingIndex::esp3d_radio_mode:
            if (value == (uint8_t)ESP3DRadioMode::off
#if ESP3D_WIFI_FEATURE
                || value == (uint8_t)ESP3DRadioMode::wifi_sta
                || value == (uint8_t)ESP3DRadioMode::wifi_ap
                || value == (uint8_t)ESP3DRadioMode::wifi_ap_config
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_BT_FEATURE
                || value == (uint8_t)ESP3DRadioMode::bluetooth_serial
                || value == (uint8_t)ESP3DRadioMode::bluetooth_ble
#endif  // ESP3D_BT_FEATURE
            )
            {
                return true;
            }
            break;
#if ESP3D_WIFI_FEATURE
        case ESP3DSettingIndex::esp3d_ap_channel:
            for (uint8_t i = 0; i < SupportedApChannelsSize; i++)
            {
                if (SupportedApChannels[i] == value)
                {
                    return true;
                }
            }
            break;
#endif  // ESP3D_WIFI_FEATURE
#if ESP3D_SD_CARD_FEATURE
#if SD_INTERFACE_TYPE == 0
        case ESP3DSettingIndex::esp3d_spi_divider:
            for (uint8_t i = 0; i < SupportedSPIDividerSize; i++)
            {
                if (SupportedSPIDivider[i] == value)
                {
                    return true;
                }
            }
            break;
#endif  // SD_INTERFACE_TYPE == 0
#endif  // ESP3D_SD_CARD_FEATURE

        case ESP3DSettingIndex::esp3d_target_firmware:
            if (static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::unknown
                || static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::grbl
                || static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::marlin
                || static_cast<ESP3DTargetFirmware>(value)
                       == ESP3DTargetFirmware::marlin_emworkspaceded
                || static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::smoothieware
                || static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::repetier
                || static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::reprap
                || static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::grblhal
                || static_cast<ESP3DTargetFirmware>(value) == ESP3DTargetFirmware::hp_gl)
                return true;

            break;
        default:
            return isValidByteTargetSetting(value, settingElement);
            break;
    }
    return false;
}
bool ESP3DSettings::isValidIPStringSetting(const char *value, ESP3DSettingIndex settingElement)
{
    const ESP3DSettingDescription *settingPtr = getSettingPtr(settingElement);
    if (!settingPtr)
    {
        return false;
    }
    if (settingPtr->type != ESP3DSettingType::ip_t)
    {
        return false;
    }
    /*return std::regex_match(value,
                            std::regex("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$"));*/
    // The below code cover more test than regex matching
    std::string ippart[4];
    uint index = 0;
    for (uint8_t i = 0; i < strlen(value); i++)
    {
        // only digit and . are allowed
        if (!isdigit(value[i]) && value[i] != '.')
        {
            return false;
        }
        if (value[i] == '.')
        {  // new ip part
            index++;
            if (index > 3)
            {  // only 4 parts allowed (IPv4 only for the moment)
                return false;
            }
        }
        else
        {  // fill the part
            ippart[index] += value[i];
        }
    }
    if (index != 3)
    {  // only exactly 4 parts allowed, no less
        return false;
    }
    for (uint8_t i = 0; i < 4; i++)
    {  // check each part
        // max value is 255, no empty part, no part longer than 3 digits
        if (atoi(ippart[i].c_str()) > 255 || ippart[i].length() > 3 || ippart[i].length() == 0)
        {
            return false;
        }
    }
    return true;
}

bool ESP3DSettings::isValidSettingsNvs()
{
    char result[SIZE_OF_SETTING_VERSION + 1] = {0};
    if (esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_version,
                                    result,
                                    SIZE_OF_SETTING_VERSION + 1))
    {
        if (!esp3dTftsettings.isValidStringSetting(result, ESP3DSettingIndex::esp3d_version))
        {
            esp3d_log_e("Expected %s but got %s", SETTING_VERSION, result);
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        esp3d_log_e("Cannot read setting version");
        return false;
    }
}

uint32_t ESP3DSettings::getDefaultIntegerSetting(ESP3DSettingIndex settingElement)
{
    const ESP3DSettingDescription *query = getSettingPtr(settingElement);
    if (query)
    {
        return (uint32_t)std::stoul(std::string(query->default_val), NULL, 0);
    }
    return 0;
}

const char *ESP3DSettings::getDefaultStringSetting(ESP3DSettingIndex settingElement)
{
    const ESP3DSettingDescription *query = getSettingPtr(settingElement);
    if (query)
    {
        return query->default_val;
    }
    return nullptr;
}

uint8_t ESP3DSettings::getDefaultByteSetting(ESP3DSettingIndex settingElement)
{
    const ESP3DSettingDescription *query = getSettingPtr(settingElement);
    if (query)
    {
        return (uint8_t)std::stoul(std::string(query->default_val), NULL, 0);
    }
    return 0;
}

const char *ESP3DSettings::GetFirmwareTargetShortName(ESP3DTargetFirmware index)
{
    switch (index)
    {
        case ESP3DTargetFirmware::grbl:
            return "grbl";
        case ESP3DTargetFirmware::marlin:
            return "marlin";
        case ESP3DTargetFirmware::marlin_emworkspaceded:
            return "marlin";
        case ESP3DTargetFirmware::smoothieware:
            return "smoothieware";
        case ESP3DTargetFirmware::repetier:
            return "repetier";
        case ESP3DTargetFirmware::reprap:
            return "reprap";
        case ESP3DTargetFirmware::grblhal:
            return "grblhal";
        case ESP3DTargetFirmware::hp_gl:
            return "hp_gl";
        default:
            break;
    }
    return "unknown";
}

bool ESP3DSettings::reset()
{
    esp3d_log("Resetting NVS");
    bool result = true;
    esp_err_t err;
    // clear all settings first
    nvs_handle_t handle_erase;
    err = nvs_open(STORAGE_NAME, NVS_READWRITE, &handle_erase);
    if (err != ESP_OK)
    {
        esp3d_log_e("Failing accessing NVS");
        return false;
    }
    if (nvs_erase_all(handle_erase) != ESP_OK)
    {
        esp3d_log_e("Failing erasing NVS");
        return false;
    }
    nvs_close(handle_erase);

    // Init each setting with default value
    // this parsing method is to workaround parsing array of enums and usage of
    // sizeof(<array of enums>) generate warning: iteration 1 invokes undefined
    // behavior [-Waggressive-loop-optimizations] may be use a vector of enum
    // would solve also fortunaly this is to reset all settings so it is ok for
    // current situation and it avoid to create an array of enums
    for (auto i : ESP3DSettingsData)
    {
        ESP3DSettingIndex setting            = i.index;
        const ESP3DSettingDescription *query = getSettingPtr(setting);
        if (query)
        {
            esp3d_log("Reseting %d value to %s",
                      static_cast<uint16_t>(setting),
                      query->default_val);
            switch (query->type)
            {
                case ESP3DSettingType::byte_t:
                    if (!ESP3DSettings::writeByte(setting,
                                                  (uint8_t)std::stoul(std::string(
                                                                          query->default_val),
                                                                      NULL,
                                                                      0)))
                    {
                        esp3d_log_e("Error writing %s to settings %d",
                                    query->default_val,
                                    static_cast<uint16_t>(setting));
                        result = false;
                    }
                    break;
                case ESP3DSettingType::ip_t:
                    if (!ESP3DSettings::writeIPString(setting, query->default_val))
                    {
                        esp3d_log_e("Error writing %s to settings %d",
                                    query->default_val,
                                    static_cast<uint16_t>(setting));
                        result = false;
                    }
                    break;
                case ESP3DSettingType::integer_t:
                    if (!ESP3DSettings::writeUint32(setting,
                                                    (uint32_t)std::stoul(std::string(
                                                                             query->default_val),
                                                                         NULL,
                                                                         0)))
                    {
                        esp3d_log_e("Error writing %s to settings %d",
                                    query->default_val,
                                    static_cast<uint16_t>(setting));
                        result = false;
                    }
                    break;
                case ESP3DSettingType::float_t:
                case ESP3DSettingType::string_t:
                    if (setting == ESP3DSettingIndex::esp3d_version)
                    {
                        if (!ESP3DSettings::writeString(setting, SETTING_VERSION))
                        {
                            esp3d_log_e("Error writing %s to settings %d",
                                        query->default_val,
                                        static_cast<uint16_t>(setting));
                            result = false;
                        }
                    }
                    else
                    {
                        if (!ESP3DSettings::writeString(setting, query->default_val))
                        {
                            esp3d_log_e("Error writing %s to settings %d",
                                        query->default_val,
                                        static_cast<uint16_t>(setting));
                            result = false;
                        }
                    }

                    break;
                default:
                    result = false;  // type is not handle
                    esp3d_log_e("Setting  %d , type %d is not handled ",
                                static_cast<uint16_t>(setting),
                                static_cast<uint8_t>(query->type));
            }
        }
        else
        {
            result = false;  // setting invalid
            esp3d_log_e("Setting  %d is unknown", static_cast<uint16_t>(setting));
        }
    }
    return result;
}

ESP3DSettings::ESP3DSettings() {}

ESP3DSettings::~ESP3DSettings() {}

bool ESP3DSettings::access_nvs(nvs_open_mode_t mode)
{
    if (mode == NVS_READONLY)
    {
        return true;
    }
#if ESP3D_PATCH_FS_ACCESS_RELEASE
    esp3d_log("apply patch");
    if (ESP_OK != bsp_accessFs())
    {
        esp3d_log_e("Error applying patch accessing FS");
    }
#endif  // ESP3D_PATCH_FS_ACCESS_RELEASE
    return true;
}

void ESP3DSettings::release_nvs(nvs_open_mode_t mode)
{
    if (mode == NVS_READONLY)
    {
        return;
    }
#if ESP3D_PATCH_FS_ACCESS_RELEASE
    esp3d_log("revert patch");
    if (ESP_OK != bsp_releaseFs())
    {
        esp3d_log_e("Error applying patch accessing FS");
    }
#endif  // ESP3D_PATCH_FS_ACCESS_RELEASE
}

uint8_t ESP3DSettings::readByte(ESP3DSettingIndex index, bool *haserror)
{
    uint8_t value                        = 0;
    const ESP3DSettingDescription *query = getSettingPtr(index);
    if (query)
    {
        if (query->type == ESP3DSettingType::byte_t)
        {
            esp_err_t err;
            access_nvs(NVS_READONLY);
            std::shared_ptr<nvs::NVSHandle> handle =
                nvs::open_nvs_handle(STORAGE_NAME, NVS_READONLY, &err);
            if (err != ESP_OK)
            {
                esp3d_log_e("Failling accessing NVS");
            }
            else
            {
                std::string key = "p_" + std::to_string((uint)query->index);
                err             = handle->get_item(key.c_str(), value);
                if (err == ESP_OK)
                {
                    if (haserror)
                    {
                        *haserror = false;
                    }
                    release_nvs(NVS_READONLY);
                    return value;
                }
                if (err == ESP_ERR_NVS_NOT_FOUND)
                {
                    value = (uint8_t)std::stoul(std::string(query->default_val), NULL, 0);
                    if (haserror)
                    {
                        *haserror = false;
                    }
                    release_nvs(NVS_READONLY);
                    return value;
                }
            }
            release_nvs(NVS_READONLY);
        }
    }
    if (haserror)
    {
        *haserror = true;
    }
    return 0;
}

uint32_t ESP3DSettings::readUint32(ESP3DSettingIndex index, bool *haserror)
{
    uint32_t value                       = 0;
    const ESP3DSettingDescription *query = getSettingPtr(index);
    if (query)
    {
        if (query->type == ESP3DSettingType::integer_t || query->type == ESP3DSettingType::ip_t)
        {
            esp_err_t err;
            access_nvs(NVS_READONLY);
            std::shared_ptr<nvs::NVSHandle> handle =
                nvs::open_nvs_handle(STORAGE_NAME, NVS_READONLY, &err);
            if (err != ESP_OK)
            {
                esp3d_log_e("Failling accessing NVS");
            }
            else
            {
                std::string key = "p_" + std::to_string((uint)query->index);
                err             = handle->get_item(key.c_str(), value);
                if (err == ESP_OK)
                {
                    if (haserror)
                    {
                        *haserror = false;
                    }
                    release_nvs(NVS_READONLY);
                    return value;
                }
                if (err == ESP_ERR_NVS_NOT_FOUND)
                {
                    if (query->type == ESP3DSettingType::integer_t)
                    {
                        value = (uint32_t)std::stoul(std::string(query->default_val), NULL, 0);
                    }
                    else
                    {  // ip is stored as uin32t but default value is ip format
                       // string
                        value = StringtoIPUInt32(query->default_val);
                    }

                    if (haserror)
                    {
                        *haserror = false;
                    }
                    release_nvs(NVS_READONLY);
                    return value;
                }
            }
            release_nvs(NVS_READONLY);
        }
    }
    if (haserror)
    {
        *haserror = true;
    }
    return 0;
}

const char *ESP3DSettings::readIPString(ESP3DSettingIndex index, bool *haserror)
{
    uint32_t ipInt    = readUint32(index, haserror);
    std::string ipStr = IPUInt32toString(ipInt);
    // esp3d_log("read setting %d : %d to %s",index, ipInt,ipStr.c_str());
    return IPUInt32toString(ipInt);
}

const char *
ESP3DSettings::readString(ESP3DSettingIndex index, char *out_str, size_t len, bool *haserror)
{
    const ESP3DSettingDescription *query = getSettingPtr(index);
    if (query != NULL)
    {
        esp3d_log("read setting %d, type: %d : %s",
                  (uint16_t)index,
                  (uint8_t)query->type,
                  query->default_val);
        if (query->type == ESP3DSettingType::string_t || query->type == ESP3DSettingType::float_t)
        {
            esp_err_t err;
            size_t setting_size = (query->size);
            if (query->type == ESP3DSettingType::float_t)
            {
                setting_size = 16;  // 15 digit + 1 (0x0) is the max size of a float
                // query->size is the precision in that case
                esp3d_log("read float setting");
            }

            if (out_str && len >= setting_size)
            {
                access_nvs(NVS_READONLY);
                std::shared_ptr<nvs::NVSHandle> handle =
                    nvs::open_nvs_handle(STORAGE_NAME, NVS_READONLY, &err);
                if (err != ESP_OK)
                {
                    esp3d_log_e("Failling accessing NVS");
                }
                else
                {
                    std::string key = "p_" + std::to_string((uint)query->index);
                    err             = handle->get_string(key.c_str(), out_str, setting_size);
                    if (err == ESP_OK)
                    {
                        if (haserror)
                        {
                            *haserror = false;
                        }
                        release_nvs(NVS_READONLY);
                        return out_str;
                    }
                    else
                    {
                        esp3d_log_w("Got error %d %s", err, esp_err_to_name(err));
                        if (err == ESP_ERR_NVS_NOT_FOUND)
                        {
                            esp3d_log_w("Not found value for %s, use default %s",
                                        key.c_str(),
                                        query->default_val);
                            strcpy(out_str, query->default_val);
                            if (haserror)
                            {
                                *haserror = false;
                            }
                            release_nvs(NVS_READONLY);
                            return out_str;
                        }
                    }
                    esp3d_log_e("Read %s failed: %s", key.c_str(), esp_err_to_name(err));
                }
                release_nvs(NVS_READONLY);
            }
            else
            {
                if (!out_str)
                {
                    esp3d_log_e("Error no output buffer");
                }
                else
                {
                    esp3d_log_e("Error size are not correct got %d but should be %d",
                                len,
                                query->size);
                }
            }
        }
        else
        {
            esp3d_log_e("Error setting is not a string / float");
        }
    }
    else
    {
        esp3d_log_e("Cannot find %d entry", static_cast<uint16_t>(index));
    }
    if (haserror)
    {
        *haserror = true;
    }
    esp3d_log_e("Error reading setting value %d", static_cast<uint16_t>(index));
    return "";
}

bool ESP3DSettings::writeByte(ESP3DSettingIndex index, const uint8_t value)
{
    const ESP3DSettingDescription *query = getSettingPtr(index);
    if (query)
    {
        if (query->type == ESP3DSettingType::byte_t)
        {
            esp_err_t err;
            access_nvs(NVS_READWRITE);
            std::shared_ptr<nvs::NVSHandle> handle =
                nvs::open_nvs_handle(STORAGE_NAME, NVS_READWRITE, &err);
            if (err != ESP_OK)
            {
                esp3d_log_e("Failling accessing NVS");
            }
            else
            {
                std::string key = "p_" + std::to_string((uint)query->index);
                if (handle->set_item(key.c_str(), value) == ESP_OK)
                {
                    release_nvs(NVS_READWRITE);
                    return (handle->commit() == ESP_OK);
                }
            }
            release_nvs(NVS_READWRITE);
        }
    }
    return false;
}

bool ESP3DSettings::writeUint32(ESP3DSettingIndex index, const uint32_t value)
{
    const ESP3DSettingDescription *query = getSettingPtr(index);
    if (query)
    {
        if (query->type == ESP3DSettingType::integer_t || query->type == ESP3DSettingType::ip_t)
        {
            esp_err_t err;
            access_nvs(NVS_READWRITE);
            std::shared_ptr<nvs::NVSHandle> handle =
                nvs::open_nvs_handle(STORAGE_NAME, NVS_READWRITE, &err);
            if (err != ESP_OK)
            {
                esp3d_log_e("Failling accessing NVS");
            }
            else
            {
                std::string key = "p_" + std::to_string((uint)query->index);
                if (handle->set_item(key.c_str(), value) == ESP_OK)
                {
                    release_nvs(NVS_READWRITE);
                    return (handle->commit() == ESP_OK);
                }
            }
            release_nvs(NVS_READWRITE);
        }
    }
    return false;
}

bool ESP3DSettings::writeIPString(ESP3DSettingIndex index, const char *byte_buffer)
{
    uint32_t ipInt    = StringtoIPUInt32(byte_buffer);
    std::string ipStr = IPUInt32toString(ipInt);
    esp3d_log("write setting %d : %s to %ld to %s",
              static_cast<uint8_t>(index),
              byte_buffer,
              ipInt,
              ipStr.c_str());

    return writeUint32(index, StringtoIPUInt32(byte_buffer));
}

bool ESP3DSettings::writeString(ESP3DSettingIndex index, const char *byte_buffer)
{
    esp3d_log("write setting %d : (%d bytes) %s ",
              static_cast<uint16_t>(index),
              strlen(byte_buffer),
              byte_buffer);
    const ESP3DSettingDescription *query = getSettingPtr(index);
    if (query)
    {
        size_t setting_size = (query->size);
        if (query->type == ESP3DSettingType::float_t)
        {
            setting_size = 16;  // 15 digit + 1 (0x0) is the max size of a float
                                // query->size is the precision in that case
        }
        if ((query->type == ESP3DSettingType::string_t || query->type == ESP3DSettingType::float_t)
            && strlen(byte_buffer) <= setting_size)
        {
            esp_err_t err;
            access_nvs(NVS_READWRITE);
            std::shared_ptr<nvs::NVSHandle> handle =
                nvs::open_nvs_handle(STORAGE_NAME, NVS_READWRITE, &err);
            if (err != ESP_OK)
            {
                esp3d_log_e("Failling accessing NVS");
            }
            else
            {
                std::string key = "p_" + std::to_string((uint)query->index);
                if (handle->set_string(key.c_str(), byte_buffer) == ESP_OK)
                {
                    esp3d_log("Write success");
                    release_nvs(NVS_READWRITE);
                    return (handle->commit() == ESP_OK);
                }
                else
                {
                    esp3d_log_e("Write failed");
                }
            }
            release_nvs(NVS_READWRITE);
        }
        else
        {
            esp3d_log_e("Incorrect valued");
        }
    }
    else
    {
        esp3d_log_e("Unknow setting");
    }
    return false;
}

const char *ESP3DSettings::IPUInt32toString(uint32_t ip_int)
{
    ip_addr_t tmpip;
    tmpip.type = IPADDR_TYPE_V4;
    // convert uint32_t to ip_addr_t
    ip_addr_set_ip4_u32_val(tmpip, ip_int);
    // convert ip_addr_t to_string
    return ip4addr_ntoa(&tmpip.u_addr.ip4);
}

uint32_t ESP3DSettings::StringtoIPUInt32(const char *s)
{
    ip_addr_t tmpip;
    esp3d_log("convert %s", s);
    tmpip.type = IPADDR_TYPE_V4;
    // convert string to ip_addr_t
    ip4addr_aton(s, &tmpip.u_addr.ip4);
    // convert ip_addr_t to uint32_t
    return ip4_addr_get_u32(&tmpip.u_addr.ip4);
}

const ESP3DSettingDescription *ESP3DSettings::getSettingPtr(const ESP3DSettingIndex index)
{
    for (uint16_t i = 0; i < sizeof(ESP3DSettingsData) / sizeof(ESP3DSettingDescription); i++)
    {
        if (ESP3DSettingsData[i].index == index)
        {
            return &ESP3DSettingsData[i];
        }
    }
    esp3d_log_e("Cannot find %d entry %s", static_cast<uint16_t>(index), get_setting_name(index));
    return nullptr;
}
