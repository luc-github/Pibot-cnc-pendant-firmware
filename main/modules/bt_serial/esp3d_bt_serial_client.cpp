/*
  esp3d_bt_serial_client

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#if ESP3D_BT_FEATURE

#include "esp3d_bt_serial_client.h"

#include <string>

#include "bt_ble_def.h"
#include "esp3d_commands.h"
#include "esp3d_hal.h"
#include "esp3d_log.h"
#include "esp3d_settings.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

ESP3DBTSerialClient btSerialClient;

bool ESP3DBTSerialClient::configure(esp3d_bt_serial_config_t *config)
{
    esp3d_log_d("Configure BT Serial Client");
    if (config)
    {
        _config = config;
        return true;
    }
    esp3d_log_e("Invalid configuration");
    return false;
}

// For the scan results
static std::vector<BTDevice> discovered_devices;
// Helper function to get the device address as a string
char *ESP3DBTSerialClient::bda2str(uint8_t *bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18)
    {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

// Helper function to convert a string MAC address to binary format
bool ESP3DBTSerialClient::str2bda(const char *str, esp_bd_addr_t bda)
{
    if (!str || !bda)
    {
        esp3d_log_e("Invalid parameters for str2bda");
        return false;
    }

    // accept several formating "AA:BB:CC:DD:EE:FF" ou "AA-BB-CC-DD-EE-FF" ou "AABBCCDDEEFF"
    int values[6];
    int result = 0;

    // Try with ':'
    result = sscanf(str,
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    &values[0],
                    &values[1],
                    &values[2],
                    &values[3],
                    &values[4],
                    &values[5]);

    // if failed try with  '-'
    if (result != 6)
    {
        result = sscanf(str,
                        "%02x-%02x-%02x-%02x-%02x-%02x",
                        &values[0],
                        &values[1],
                        &values[2],
                        &values[3],
                        &values[4],
                        &values[5]);
    }

    // if still failed try without any separator
    if (result != 6)
    {
        result = sscanf(str,
                        "%02x%02x%02x%02x%02x%02x",
                        &values[0],
                        &values[1],
                        &values[2],
                        &values[3],
                        &values[4],
                        &values[5]);
    }

    if (result != 6)
    {
        esp3d_log_e("Invalid MAC address format: %s (expected XX:XX:XX:XX:XX:XX)", str);
        return false;
    }

    // Check if values are in valid range
    for (int i = 0; i < 6; i++)
    {
        if (values[i] < 0 || values[i] > 255)
        {
            esp3d_log_e("Invalid MAC address byte at position %d: %02x", i, values[i]);
            return false;
        }
        bda[i] = (uint8_t)values[i];
    }

    esp3d_log_d("Converted MAC %s to binary format", str);
    return true;
}

// Helper to get the device name from EIR data
bool ESP3DBTSerialClient::get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname    = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir)
    {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname)
    {
        rmt_bdname =
            esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname)
    {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
        {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname)
        {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len)
        {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

// Callback function for Bluetooth GAP events
static void esp_bt_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    btSerialClient.esp_bt_gap_cb(event, param);
}

// Callback function for Bluetooth SPP events
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    btSerialClient.sppCallback(event, param);
}

// Handle Bluetooth GAP events
void ESP3DBTSerialClient::esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
            // Handle device discovery results
            // In esp_bt_gap_cb, case ESP_BT_GAP_DISC_RES_EVT:
        case ESP_BT_GAP_DISC_RES_EVT: {
            BTDevice device;
            memcpy(device.addr, param->disc_res.bda, ESP_BD_ADDR_LEN);
            device.name = "";

            // Check if the device already exists in the discovered devices list
            bool device_exists = false;
            for (const auto &existing_device : discovered_devices)
            {
                if (memcmp(existing_device.addr, device.addr, ESP_BD_ADDR_LEN) == 0)
                {
                    device_exists = true;
                    break;
                }
            }

            // If the device does not exist, add it to the list
            if (!device_exists)
            {
                for (int i = 0; i < param->disc_res.num_prop; i++)
                {
                    if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR)
                    {
                        uint8_t *eir  = (uint8_t *)param->disc_res.prop[i].val;
                        uint8_t len   = param->disc_res.prop[i].len;
                        char name[32] = {0};
                        if (get_name_from_eir(eir, name, &len))
                        {
                            device.name = name;
                        }
                    }
                }
                discovered_devices.push_back(device);
                esp3d_log_d("Found new device: %s (%02X:%02X:%02X:%02X:%02X:%02X)",
                            device.name.c_str(),
                            device.addr[0],
                            device.addr[1],
                            device.addr[2],
                            device.addr[3],
                            device.addr[4],
                            device.addr[5]);
            }
            else
            {
                // Device already exists, update its name if necessary
                for (auto &existing_device : discovered_devices)
                {
                    if (memcmp(existing_device.addr, device.addr, ESP_BD_ADDR_LEN) == 0)
                    {
                        if (existing_device.name.empty())
                        {
                            for (int i = 0; i < param->disc_res.num_prop; i++)
                            {
                                if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR)
                                {
                                    uint8_t *eir  = (uint8_t *)param->disc_res.prop[i].val;
                                    uint8_t len   = param->disc_res.prop[i].len;
                                    char name[32] = {0};
                                    if (get_name_from_eir(eir, name, &len))
                                    {
                                        existing_device.name = name;
                                        esp3d_log_d("Updated device name: %s "
                                                    "(%02X:%02X:%02X:%02X:%02X:%02X)",
                                                    name,
                                                    device.addr[0],
                                                    device.addr[1],
                                                    device.addr[2],
                                                    device.addr[3],
                                                    device.addr[4],
                                                    device.addr[5]);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
            break;
        }

        case ESP_BT_GAP_READ_REMOTE_NAME_EVT:
            if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS)
            {
                char remote_device_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = {0};
                strncpy(remote_device_name,
                        (char *)param->read_rmt_name.rmt_name,
                        ESP_BT_GAP_MAX_BDNAME_LEN);
                remote_device_name[ESP_BT_GAP_MAX_BDNAME_LEN] = '\0';
                _current_name = remote_device_name;  // Store the name for later use
                esp3d_log_d("Remote device name: %s", remote_device_name);
            }
            else
            {
                esp3d_log_e("Failed to read remote device name");
                _current_name = "Unknown";  // Set a default name if reading fails
            }
            break;
            // Handle discovery state changes
            // Dans esp_bt_gap_cb, case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
            esp3d_log_d("Discovery state changed: %d", param->disc_st_chg.state);
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED)
            {
                esp3d_log_d("BT scan started");
                discovered_devices.clear();
                _discovery_started = true;
                _scan_completed    = false;
            }
            else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED)
            {
                _discovery_started = false;
                esp3d_log_d("BT scan stopped, found %d devices", discovered_devices.size());

                // Store the discovered devices in the last scan results
                _last_scan_results = discovered_devices;

#if ESP3D_TFT_LOG
                for (const auto &device : discovered_devices)
                {
                    esp3d_log_d("Final device: %s (%02X:%02X:%02X:%02X:%02X:%02X)",
                                device.name.c_str(),
                                device.addr[0],
                                device.addr[1],
                                device.addr[2],
                                device.addr[3],
                                device.addr[4],
                                device.addr[5]);
                }
#endif
                // Set the scan completed flag
                _scan_completed = true;
            }
            break;
        }
            // Handle remote service discovery events
        case ESP_BT_GAP_RMT_SRVCS_EVT: {
            esp3d_log_d("Remote services event received");
            if (param->rmt_srvcs.stat == ESP_BT_STATUS_SUCCESS)
            {
                esp3d_log_d("Remote services discovered: %d", param->rmt_srvcs.num_uuids);
                // Iterate through the discovered services
                for (int i = 0; i < param->rmt_srvcs.num_uuids; i++)
                {
                    esp_bt_uuid_t uuid =
                        param->rmt_srvcs.uuid_list[i];  // Accès direct, pas de pointeur
                    if (uuid.len == ESP_UUID_LEN_16 && uuid.uuid.uuid16 == 0x1101)
                    {
                        esp3d_log_d("Found SPP service");
                    }
                }
            }
            else
            {
                esp3d_log_e("Remote services discovery failed, status: %d", param->rmt_srvcs.stat);
            }
            break;
        }
        // Handle remote service record events
        // This event is triggered when a specific service record is found
        // It is usually not necessary for SPP, but can be useful for other profiles
        // It provides details about a specific service record
        // such as its UUID and attributes
        // This event is typically used for profiles other than SPP
        // For SPP, you may not need to handle this event
        case ESP_BT_GAP_RMT_SRVC_REC_EVT:
            esp3d_log_d("Remote service record event received");
            break;
        // Handle authentication completion
        // This event is triggered when a device is authenticated
        case ESP_BT_GAP_AUTH_CMPL_EVT: {
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
            {
                esp3d_log_d("authentication success: %s", param->auth_cmpl.device_name);
            }
            else
            {
                esp3d_log_e("authentication failed, status:%d", param->auth_cmpl.stat);
            }
            break;
        }
        // Handle authentication
        case ESP_BT_GAP_CFM_REQ_EVT:
            esp3d_log_d("ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %06" PRIu32,
                        param->cfm_req.num_val);
            esp3d_log_d("To confirm the value, type `spp ok;`");
            break;
            // Handle key notification event
            // This event is triggered when a passkey is notified
            // It indicates that the device is requesting a passkey for pairing
            // You can display the passkey to the user for confirmation
            // This event is typically used for pairing devices that require a passkey
            // It provides the passkey that the user should confirm
            // It is typically used for pairing devices that require a passkey

        case ESP_BT_GAP_KEY_NOTIF_EVT:
            esp3d_log_d("ESP_BT_GAP_KEY_NOTIF_EVT passkey:%06" PRIu32, param->key_notif.passkey);
            esp3d_log_d("Waiting response...");
            break;
            // Handle key request event
        // This event is triggered when a passkey is requested
        // It indicates that the device requires a passkey for pairing
        // You can prompt the user to enter the passkey
        // and respond with the appropriate command
        // This event is typically used for pairing devices that require a passkey
        case ESP_BT_GAP_KEY_REQ_EVT:
            esp3d_log_d("ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
            esp3d_log_d("To input the key, type `spp key xxxxxx;`");
            break;

            // Handle mode change events
            // This event is triggered when the Bluetooth mode changes
            // It indicates that the Bluetooth mode has changed
            // This event is typically used to monitor changes in Bluetooth mode
            // It provides information about the new mode Discoverable, Connectable, etc.
        case ESP_BT_GAP_MODE_CHG_EVT:
            esp3d_log_d("ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
            break;
        default:
            break;
    }
}

// Handle Bluetooth SPP events
void ESP3DBTSerialClient::sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event)
    {

        // Handle SPP initialization event in client mode
        case ESP_SPP_CL_INIT_EVT:
            esp3d_log_d("SPP Connection opened, handle: %d", (int)param->srv_open.handle);
            _spp_handle = param->srv_open.handle;
            if (param->init.status == ESP_SPP_SUCCESS)
            {
                esp3d_log_d("ESP_SPP_INIT_EVT - SPP initialized successfully, handle: %d",
                            _spp_handle);
            }
            else
            {
                esp3d_log_d("ESP_SPP_INIT_EVT status:%d", param->init.status);
            }
            break;
            // Handle SPP open event inb client mode
        case ESP_SPP_OPEN_EVT: {
            esp3d_log_d("SPP Connection opened, handle: %d", (int)param->open.handle);
            if (param->open.status == ESP_SPP_SUCCESS)
            {
                _spp_handle = param->open.handle;
                esp3d_log_d("ESP_SPP_OPEN_EVT - SPP opened successfully, handle: %d", _spp_handle);
                char addr_str[18] = {0};
                bda2str(param->open.rem_bda, addr_str, sizeof(addr_str));
                _current_address = addr_str;  // Store the address for later use
                _current_name.clear();        // Clear the current name as it will be updated later
                esp3d_log_d("Connected to device address: %s", addr_str);
                esp_err_t ret = esp_spp_write(_spp_handle, strlen("Hello"), (uint8_t *)"Hello");
                // Check if the write operation was successful
                if (ret != ESP_OK)
                {
                    esp3d_log_e("Error writing message : %s", esp_err_to_name(ret));
                }
                // get remote name
                ret = esp_bt_gap_read_remote_name(param->open.rem_bda);
                if (ret != ESP_OK)
                {
                    esp3d_log_e("Failed to read remote name: %s", esp_err_to_name(ret));
                    _current_name = "Unknown";  // Set a default name if reading fails
                }
            }
            else
            {
                esp3d_log_d("ESP_SPP_OPEN_EVT status:%d", param->open.status);
            }
            break;
        }
        // Handle SPP close event
        case ESP_SPP_CLOSE_EVT:
            esp3d_log_d("ESP_SPP_CLOSE_EVT status:%d handle:%" PRIu32 " close_by_remote:%d",
                        param->close.status,
                        param->close.handle,
                        param->close.async);
            _spp_handle  = -1;
            _rxBufferPos = 0;  // Reset the RX buffer position
            _current_name.clear();
            _current_address.clear();
            break;
        // SPP  discovery complete event
        case ESP_SPP_DISCOVERY_COMP_EVT:
            if (param->disc_comp.status == ESP_SPP_SUCCESS)
            {
                esp3d_log_d("ESP_SPP_DISCOVERY_COMP_EVT scn_num:%d", param->disc_comp.scn_num);
                for (uint8_t i = 0; i < param->disc_comp.scn_num; i++)
                {
                    esp3d_log_d("-- [%d] scn:%d service_name:%s",
                                i,
                                param->disc_comp.scn[i],
                                param->disc_comp.service_name[i]);
                }
            }
            else
            {
                esp3d_log_d("ESP_SPP_DISCOVERY_COMP_EVT status=%d", param->disc_comp.status);
            }
            break;
        // Handle Start event in client mode
        // This event is triggered when the SPP service starts
        // It indicates that the SPP service has started successfully
        // You can use this event to perform any necessary initialization
        // or to notify the user that the SPP service is ready
        // This event is typically used to indicate that the SPP service has started
        // and is ready to accept connections or data
        case ESP_SPP_START_EVT:
            esp3d_log("ESP_SPP_START_EVT");
            break;

            // Handle SPP data reception event
            // This event is triggered when data is received over the SPP connection
            // It indicates that data has been received and is available for processing

        case ESP_SPP_DATA_IND_EVT: {
            for (size_t i = 0; i < param->data_ind.len; i++)
            {
                esp3d_log_d("Received data: %02X", param->data_ind.data[i]);
                if (_rxBufferPos < _config->rx_buffer_size)
                {
                    _rxBuffer[_rxBufferPos++] = param->data_ind.data[i];
                }
                else
                {
                    esp3d_log_e("Rx buffer overflow");
                    _rxBufferPos = 0;
                    break;
                }
                if (isEndChar(param->data_ind.data[i]))
                {
                    esp3d_log_d("End character detected, pushing message to RX queue");
                    pushMsgToRxQueue(_rxBuffer, _rxBufferPos);
                    _rxBufferPos = 0;
                }
            }
            break;
        }
            // Handle SPP write event
            // This event is triggered when data is written to the SPP connection
            // It indicates that data has been successfully written or that there was an error

        case ESP_SPP_WRITE_EVT:
            if (param->write.status == ESP_SPP_SUCCESS)
            {
                esp3d_log_d("ESP_SPP_WRITE_EVT status:%d handle:%" PRIu32 " len:%d ",
                            param->write.status,
                            param->write.handle,
                            param->write.len);
            }
            else
            {
                esp3d_log_e("ESP_SPP_WRITE_EVT status:%d", param->write.status);
            }
            break;
        // Handle SPP Congestion event
        // This event is triggered when the SPP connection is congested
        // It indicates that the SPP connection is congested and cannot accept more data
        // You can use this event to pause sending data until the congestion is resolved
        // This event is typically used to manage data flow in the SPP connection
        case ESP_SPP_CONG_EVT:
            esp3d_log_d("ESP_SPP_CONG_EVT status:%d handle:%" PRIu32 " ",
                        param->cong.status,
                        param->cong.handle);
            if (param->cong.cong == 0)
            {
                /* Send the privous (partial) data packet or the next data packet. */
                // esp_spp_write(param->write.handle, spp_data + SPP_DATA_LEN - s_p_data, s_p_data);
            }
            break;
        // Handle SPP Server open event
        // This event is triggered when the SPP server is opened
        // It indicates that the SPP server has been successfully opened and is ready to accept
        // connections You can use this event to perform any necessary initialization or to notify
        // the user that the SPP server is ready
        case ESP_SPP_SRV_OPEN_EVT:
            esp3d_log_d("ESP_SPP_SRV_OPEN_EVT");
            break;
            // Handle SPP Uninit event
            // This event is triggered when the SPP service is uninitialized
            // It indicates that the SPP service has been uninitialized and is no longer available
        case ESP_SPP_UNINIT_EVT:
            esp3d_log_d("ESP_SPP_UNINIT_EVT");
            break;
        default:
            break;
    }
}

// Constructor
ESP3DBTSerialClient::ESP3DBTSerialClient()
{
    _started           = false;
    _config            = NULL;
    _discovery_started = false;
    _scan_completed    = false;
    _spp_handle        = -1;
    _rxBuffer          = NULL;
    _rxBufferPos       = 0;
    pthread_mutex_init(&_tx_mutex, NULL);
    pthread_mutex_init(&_rx_mutex, NULL);
}

// Destructor
ESP3DBTSerialClient::~ESP3DBTSerialClient()
{
    end();
    pthread_mutex_destroy(&_tx_mutex);
    pthread_mutex_destroy(&_rx_mutex);
}

// Process incoming messages
void ESP3DBTSerialClient::process(ESP3DMessage *msg)
{
    esp3d_log_d("Processing message for BT Serial");
    if (!msg)
    {
        esp3d_log_e("Null message");
        return;
    }
    if (!isConnected())
    {
        esp3d_log_w("BT Serial not connected, discarding message");
        ESP3DClient::deleteMsg(msg);
        return;
    }
    if (!addTxData(msg))
    {
        flush();
        if (!addTxData(msg))
        {
            esp3d_log_e("Cannot add msg to client queue");
            ESP3DClient::deleteMsg(msg);
        }
    }
    else
    {
        flush();
    }
}

// Function to check if a character is the end character
//  Note: do we need to change this to Macro?
bool ESP3DBTSerialClient::isEndChar(uint8_t ch)
{
    return ((char)ch == '\n');
}

bool ESP3DBTSerialClient::clearBondedDevices()
{

    int expected_dev_num = esp_bt_gap_get_bond_device_num();
    if (expected_dev_num == 0)
    {
        esp3d_log_d("No bonded devices found");
        return true;
    }
    esp3d_log_d("Found %d bonded devices", expected_dev_num);

    esp_bd_addr_t *dev_list = (esp_bd_addr_t *)malloc(sizeof(esp_bd_addr_t) * expected_dev_num);
    if (dev_list == NULL)
    {
        esp3d_log_e("Could not allocate buffer for bonded devices");
        return false;
    }

    int dev_num;
    esp_err_t ret = esp_bt_gap_get_bond_device_list(&dev_num, dev_list);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to get bonded device list: %s", esp_err_to_name(ret));
        free(dev_list);
        return false;
    }

    if (dev_num != expected_dev_num)
    {
        esp3d_log_w("Inconsistent number of bonded devices. Expected %d, returned %d", expected_dev_num, dev_num);
    }

    bool success = true;
    for (int i = 0; i < dev_num; i++)
    {
        char addr_str[18] = {0};
        bda2str(dev_list[i], addr_str, sizeof(addr_str));
        ret = esp_bt_gap_remove_bond_device(dev_list[i]);
        if (ret == ESP_OK)
        {
            esp3d_log_d("Removed bonded device #%d: %s", i, addr_str);
        }
        else
        {
            esp3d_log_e("Failed to remove bonded device #%d: %s, error: %s", i, addr_str, esp_err_to_name(ret));
            success = false;
        }
    }

    free(dev_list);
    esp3d_log_d("Remaining bonded devices: %d", esp_bt_gap_get_bond_device_num());
    return success;
}

// Begin the Bluetooth Serial Client
bool ESP3DBTSerialClient::begin()
{
    end();
    // retreive configuration
    configure(&esp3dBTSerialConfig);
    if (!_config)
    {
        esp3d_log_e("BT Serial not configured");
        return false;
    }
    // Allocate memory for  rx buffer
    _rxBuffer = (uint8_t *)malloc(_config->rx_buffer_size);
    if (!_rxBuffer)
    {
        esp3d_log_e("Failed to allocate memory for rx buffer");
        return false;
    }
    _rxBufferPos = 0;
    // Initialize mutexes for thread safety
    if (pthread_mutex_init(&_rx_mutex, NULL) != 0)
    {
        free(_rxBuffer);
        _rxBuffer = NULL;
        esp3d_log_e("Mutex creation for rx failed");
        return false;
    }
    setRxMutex(&_rx_mutex);
    // Initialize mutex for tx
    if (pthread_mutex_init(&_tx_mutex, NULL) != 0)
    {
        free(_rxBuffer);
        _rxBuffer = NULL;
        pthread_mutex_destroy(&_rx_mutex);
        esp3d_log_e("Mutex creation for tx failed");
        return false;
    }
    setTxMutex(&_tx_mutex);

    esp_err_t ret = ESP_OK;
    // Release any previously allocated BT BLE memory
    ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to release BLE memory: %s", esp_err_to_name(ret));
        return false;
    }
    else
    {
        esp3d_log_d("BLE memory released successfully");
    }

    // Initialize the Bluetooth controller
    esp3d_log_d("Initializing Bluetooth controller");
    esp_bt_controller_status_t status = esp_bt_controller_get_status();
    esp3d_log_d("Bluetooth controller status: %d", status);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.mode                       = ESP_BT_MODE_CLASSIC_BT;  // Set to classic BT mode
    esp3d_log_d("BT config - mode: %d, tx_power: %d",
                bt_cfg.controller_task_stack_size,
                bt_cfg.controller_task_prio);
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to init BT controller:%s", esp_err_to_name(ret));
        return false;
    }
    // Enable the Bluetooth controller in classic mode (BT Serial)
    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to enable BT controller : %s", esp_err_to_name(ret));
        return false;
    }

    // Initialize Bluedroid stack
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    // Disable SSP (Secure Simple Pairing) for classic Bluetooth
    bluedroid_cfg.ssp_en = false;

    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to init Bluedroid %s:", esp_err_to_name(ret));
        return false;
    }
    // Enable Bluedroid stack
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to enable Bluedroid %s:", esp_err_to_name(ret));
        return false;
    }

    // Register the GAP callback function (Discovery, connection events)
    ret = esp_bt_gap_register_callback(esp_bt_gap_callback);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to register GAP callback:%s", esp_err_to_name(ret));
        return false;
    }

    // Set the Bluetooth device name
    char hostname[32] = "PibotCNC";
    esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_hostname, hostname, sizeof(hostname));
    strncpy(_config->device_name, hostname, sizeof(_config->device_name) - 1);
    _config->device_name[sizeof(_config->device_name) - 1] = '\0';
    esp_bt_gap_set_device_name(_config->device_name);

    // Register the SPP callback function (for handling SPP events, sending data, etc.)
    ret = esp_spp_register_callback(esp_spp_cb);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to register SPP callback :%s", esp_err_to_name(ret));
        ;
        return false;
    }

    // Initialize SPP (Serial Port Profile) with the specified configuration
    esp_spp_cfg_t spp_config = {
        .mode              = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = false,  // Allow more kind of devices to be connected but less reliable
        .tx_buffer_size    = 0,      // Only used for ES_SSP_MODE_VFS
    };
    ret = esp_spp_enhanced_init(&spp_config);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to init SPP  :%s", esp_err_to_name(ret));
        return false;
    }
    // Note :  esp_bt_gap_set_scan_modeis not needed because it is the one which connect to the
    // target device

    _started = true;
    flush();
    esp3d_log_d("BT Serial Client started successfully");
    clearBondedDevices(); // Supprimer les appareils appairés
    esp3d_hal::wait(1000);
    // Start the connection process
    connect();
    if (!isConnected())
    {
        esp3d_log_e("Failed to start connect to Bluetooth device");
    }
    return true;
}

bool ESP3DBTSerialClient::connect()
{
    if (!_started)
    {
        esp3d_log_e("BT Serial not started");
        return false;
    }
    bool connection_started = false;
    esp_bd_addr_t addr;
    char out_str[18] = {0};
    // Retrieve the Bluetooth address from settings
    std::string addr_str =
        esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_btserial_address, out_str, 18);
    _current_name.clear();
    _current_address.clear();
    // Check if the address is valid to save time
    if (!esp3dTftsettings.isValidStringSetting(addr_str.c_str(),
                                               ESP3DSettingIndex::esp3d_btserial_address))
    {
        esp3d_log_e("Invalid Bluetooth address setting");
        return connection_started;
    }
    // Convert the address string to binary format
    if (!str2bda(addr_str.c_str(), addr))
    {
        esp3d_log_e("Failed to convert address string to binary format");
        return connection_started;
    }
    // Now try to connect to the device
    esp3d_log_d("Connecting to Bluetooth device: %s", addr_str.c_str());
    esp_err_t ret =
        esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER, _config->spp_channel, addr);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to initiate SPP connection : %s", esp_err_to_name(ret));
        return connection_started;
    }
    esp3d_log_d("Initiated SPP client connection");
    connection_started = true;
    _current_address   = addr_str;
    return connection_started;
}

bool ESP3DBTSerialClient::pushMsgToRxQueue(const uint8_t *msg, size_t size)
{
    ESP3DMessage *newMsgPtr = newMsg();
    if (newMsgPtr)
    {
        if (ESP3DClient::setDataContent(newMsgPtr, msg, size))
        {   
            esp3d_log_d("Pushing message to RX queue, size: %s", (const char *)newMsgPtr->data);
            newMsgPtr->origin = ESP3DClientType::bt_serial;
            newMsgPtr->type   = ESP3DMessageType::unique;
            if (!addRxData(newMsgPtr))
            {
                ESP3DClient::deleteMsg(newMsgPtr);
                esp3d_log_e("Failed to add message to rx queue");
                return false;
            }
        }
        else
        {
            free(newMsgPtr);
            esp3d_log_e("Message creation failed");
            return false;
        }
    }
    else
    {
        esp3d_log_e("Out of memory!");
        return false;
    }
    return true;
}

void ESP3DBTSerialClient::handle()
{
    if (_started && isConnected())
    {
        if (getRxMsgsCount() > 0)
        {
            esp3d_log_d("Got %d messages in RX queue", getRxMsgsCount());
            ESP3DMessage *msg = popRx();
            if (msg)
            {
                esp3d_log_d("Processing message from RX queue: %s", msg->data);
                esp3dCommands.process(msg);
            }
        }
        if (getTxMsgsCount() > 0)
        {
            esp3d_log_d("Got %d messages in TX queue", getTxMsgsCount());
            ESP3DMessage *msg = popTx();
            if (msg)
            {
                esp3d_log_d("Sending message from TX queue: %s", msg->data);
                esp_err_t ret = esp_spp_write(_spp_handle, msg->size, msg->data);
                if (ret != ESP_OK)
                {
                    esp3d_log_e("Error writing message : %s", esp_err_to_name(ret));
                }
                deleteMsg(msg);
            }
        }
    } 
}

void ESP3DBTSerialClient::flush()
{
    uint8_t loopCount = 10;
    while (loopCount && getTxMsgsCount() > 0)
    {
        loopCount--;
        handle();
        esp3d_hal::wait(50);
    }
}

void ESP3DBTSerialClient::end()
{
    if (_started)
    {
        flush();
        _started = false;
        clearRxQueue();
        clearTxQueue();
        esp3d_hal::wait(1000);
        _spp_handle = -1;  // Reset SPP handle
        if (pthread_mutex_destroy(&_tx_mutex) != 0)
        {
            esp3d_log_w("Mutex destruction for tx failed");
        }
        if (pthread_mutex_destroy(&_rx_mutex) != 0)
        {
            esp3d_log_w("Mutex destruction for rx failed");
        }
        esp_spp_deinit();
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
    }

    if (_rxBuffer)
    {
        free(_rxBuffer);
        _rxBuffer = NULL;
    }
    _rxBufferPos = 0;
}

bool ESP3DBTSerialClient::scan(std::vector<BTDevice> &devices)
{
    if (!_started)
    {
        esp3d_log_e("BT Serial not started");
        return false;
    }

    discovered_devices.clear();
    _last_scan_results.clear();
    _scan_completed = false;  // Reset du flag

    esp_err_t ret = esp_bt_gap_register_callback(esp_bt_gap_callback);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to register GAP callback : %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, _config->scan_duration, 0);
    if (ret != ESP_OK)
    {
        esp3d_log_e("Failed to start discovery  : %s", esp_err_to_name(ret));
        return false;
    }

    // Wait for the scan to complete
    uint32_t timeout_ms              = (_config->scan_duration + 2) * 1000;
    uint32_t elapsed_ms              = 0;
    const uint32_t check_interval_ms = 100;

    while (!_scan_completed && elapsed_ms < timeout_ms)
    {
        esp3d_hal::wait(check_interval_ms);
        elapsed_ms += check_interval_ms;
    }

    if (!_scan_completed)
    {
        esp3d_log_w("Scan timeout reached");
        esp_bt_gap_cancel_discovery();
        return false;
    }

    // Copy the discovered devices to the output vector
    devices = _last_scan_results;

    esp3d_log_d("Scan completed with %d unique devices", devices.size());
    return true;
}

bool ESP3DBTSerialClient::getDeviceAddress(const std::string &name, esp_bd_addr_t &addr)
{
    for (const auto &device : _last_scan_results)
    {
        if (device.name == name)
        {
            memcpy(addr, device.addr, ESP_BD_ADDR_LEN);
            return true;
        }
    }
    esp3d_log_e("Device %s not found in last scan", name.c_str());
    return false;
}

#endif  // ESP3D_BT_FEATURE