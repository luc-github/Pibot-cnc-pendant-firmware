/*
  esp3d_bt_serial_client

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#if ESP3D_BT_FEATURE

#include "esp3d_bt_serial_client.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "esp3d_commands.h"
#include "esp3d_hal.h"
#include "esp3d_log.h"
#include "esp3d_settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bt_ble_def.h"
#include <string>

ESP3DBTSerialClient btSerialClient;

bool ESP3DBTSerialClient::configure(esp3d_bt_serial_config_t* config) {
  esp3d_log("Configure BT Serial Client");
  if (config) {
    _config = config;
    return true;
  }
  esp3d_log_e("Invalid configuration");
  return false;
}

static std::vector<BTDevice> discovered_devices;

static void esp_bt_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
  btSerialClient.esp_bt_gap_cb(event, param);
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  btSerialClient.sppCallback(event, param);
}

void ESP3DBTSerialClient::sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  switch (event) {
    case ESP_SPP_SRV_OPEN_EVT:
    case ESP_SPP_CL_INIT_EVT:
      esp3d_log("SPP Connection opened, handle: %d", param->srv_open.handle);
      _spp_handle = param->srv_open.handle;
      break;
    case ESP_SPP_CLOSE_EVT:
      esp3d_log("SPP Connection closed");
      _spp_handle = -1;
      break;
    case ESP_SPP_DATA_IND_EVT: {
      for (size_t i = 0; i < param->data_ind.len; i++) {
        if (_rxBufferPos < _config->rx_buffer_size) {
          _rxBuffer[_rxBufferPos++] = param->data_ind.data[i];
        } else {
          esp3d_log_e("Rx buffer overflow");
          _rxBufferPos = 0;
          break;
        }
        if (isEndChar(param->data_ind.data[i])) {
          pushMsgToRxQueue(_rxBuffer, _rxBufferPos);
          _rxBufferPos = 0;
        }
      }
      break;
    }
    default:
      break;
  }
}

// Helper function to parse EIR data and extract device name
static bool get_name_from_eir(uint8_t* eir, char* name, uint8_t len) {
  if (!eir || len == 0) return false;

  uint8_t* ptr = eir;
  while (ptr < eir + len) {
    uint8_t field_len = ptr[0];
    if (field_len == 0) break; // End of EIR data
    uint8_t field_type = ptr[1];

    if (field_type == ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME || 
        field_type == ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME) {
      uint8_t name_len = field_len - 1; // Subtract type byte
      if (name_len > 31) name_len = 31; // Truncate to fit buffer
      memcpy(name, ptr + 2, name_len);
      name[name_len] = '\0';
      return true;
    }
    ptr += field_len + 1;
  }
  return false;
}

void ESP3DBTSerialClient::esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
  switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
      BTDevice device;
      memcpy(device.addr, param->disc_res.bda, ESP_BD_ADDR_LEN);
      device.name = "";
      for (int i = 0; i < param->disc_res.num_prop; i++) {
        if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR) {
          uint8_t* eir = (uint8_t*)param->disc_res.prop[i].val;
          uint8_t len = param->disc_res.prop[i].len;
          char name[32] = {0};
          if (get_name_from_eir(eir, name, len)) {
            device.name = name;
          }
        }
      }
      discovered_devices.push_back(device);
      esp3d_log("Found device: %s (%02X:%02X:%02X:%02X:%02X:%02X)",
                device.name.c_str(),
                device.addr[0], device.addr[1], device.addr[2],
                device.addr[3], device.addr[4], device.addr[5]);
      break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
      if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
        esp3d_log("BT scan completed");
      }
      break;
    }
    default:
      break;
  }
}

ESP3DBTSerialClient::ESP3DBTSerialClient() {
  _started = false;
  _data = NULL;
  _config = NULL;
  _spp_handle = -1;
  _rxBuffer = NULL;
  _rxBufferPos = 0;
}

ESP3DBTSerialClient::~ESP3DBTSerialClient() {
  end();
}

void ESP3DBTSerialClient::process(ESP3DMessage* msg) {
  esp3d_log("Processing message for BT Serial");
  if (!msg) {
    esp3d_log_e("Null message");
    return;
  }
  if (!isConnected()) {
    esp3d_log_w("BT Serial not connected, discarding message");
    ESP3DClient::deleteMsg(msg);
    return;
  }
  if (!addTxData(msg)) {
    flush();
    if (!addTxData(msg)) {
      esp3d_log_e("Cannot add msg to client queue");
      ESP3DClient::deleteMsg(msg);
    }
  } else {
    flush();
  }
}

bool ESP3DBTSerialClient::isEndChar(uint8_t ch) {
  return ((char)ch == '\n');
}

bool ESP3DBTSerialClient::begin() {
  end();
  configure(&esp3dBTSerialConfig);
  if (!_config) {
    esp3d_log_e("BT Serial not configured");
    return false;
  }

  _data = (uint8_t*)malloc(_config->rx_buffer_size);
  if (!_data) {
    esp3d_log_e("Failed to allocate memory for data buffer");
    return false;
  }
  _rxBuffer = (uint8_t*)malloc(_config->rx_buffer_size);
  if (!_rxBuffer) {
    free(_data);
    _data = NULL;
    esp3d_log_e("Failed to allocate memory for rx buffer");
    return false;
  }
  _rxBufferPos = 0;

  if (pthread_mutex_init(&_rx_mutex, NULL) != 0) {
    free(_data);
    free(_rxBuffer);
    _data = NULL;
    _rxBuffer = NULL;
    esp3d_log_e("Mutex creation for rx failed");
    return false;
  }
  setRxMutex(&_rx_mutex);

  if (pthread_mutex_init(&_tx_mutex, NULL) != 0) {
    free(_data);
    free(_rxBuffer);
    _data = NULL;
    _rxBuffer = NULL;
    pthread_mutex_destroy(&_rx_mutex);
    esp3d_log_e("Mutex creation for tx failed");
    return false;
  }
  setTxMutex(&_tx_mutex);

  esp_err_t ret;
  ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to release BLE memory");
    return false;
  }

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to init BT controller");
    return false;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to enable BT controller");
    return false;
  }

  ret = esp_bluedroid_init();
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to init Bluedroid");
    return false;
  }

  ret = esp_bluedroid_enable();
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to enable Bluedroid");
    return false;
  }

  ret = esp_spp_register_callback(esp_spp_cb);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to register SPP callback");
    return false;
  }

  esp_spp_cfg_t spp_config = {
      .mode = ESP_SPP_MODE_CB,
      .enable_l2cap_ertm = false,
      .tx_buffer_size = 0,
  };
  ret = esp_spp_enhanced_init(&spp_config);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to init SPP");
    return false;
  }

  char hostname[32] = "PibotCNC";
  esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_hostname, hostname, sizeof(hostname));
  strncpy(_config->device_name, hostname, sizeof(_config->device_name) - 1);
  _config->device_name[sizeof(_config->device_name) - 1] = '\0';
  esp_bt_gap_set_device_name(_config->device_name);
  esp_bt_gap_set_scan_mode(_config->connectable ? ESP_BT_CONNECTABLE : ESP_BT_NON_CONNECTABLE,
                           _config->discoverable ? ESP_BT_GENERAL_DISCOVERABLE : ESP_BT_NON_DISCOVERABLE);

  _started = true;
  flush();
  return true;
}

bool ESP3DBTSerialClient::connect(esp_bd_addr_t addr) {
  if (!_started) {
    esp3d_log_e("BT Serial not started");
    return false;
  }
  esp_err_t ret = esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER,
                                  _config->spp_channel, addr);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to initiate SPP connection");
    return false;
  }
  esp3d_log("Initiated SPP client connection");
  return true;
}

bool ESP3DBTSerialClient::pushMsgToRxQueue(const uint8_t* msg, size_t size) {
  ESP3DMessage* newMsgPtr = newMsg();
  if (newMsgPtr) {
    if (ESP3DClient::setDataContent(newMsgPtr, msg, size)) {
      newMsgPtr->origin = ESP3DClientType::bt_serial;
      newMsgPtr->type = ESP3DMessageType::unique;
      if (!addRxData(newMsgPtr)) {
        ESP3DClient::deleteMsg(newMsgPtr);
        esp3d_log_e("Failed to add message to rx queue");
        return false;
      }
    } else {
      free(newMsgPtr);
      esp3d_log_e("Message creation failed");
      return false;
    }
  } else {
    esp3d_log_e("Out of memory!");
    return false;
  }
  return true;
}

void ESP3DBTSerialClient::handle() {
  if (_started && isConnected()) {
    if (getRxMsgsCount() > 0) {
      ESP3DMessage* msg = popRx();
      if (msg) {
        esp3dCommands.process(msg);
      }
    }
    if (getTxMsgsCount() > 0) {
      ESP3DMessage* msg = popTx();
      if (msg) {
        esp_err_t ret = esp_spp_write(_spp_handle, msg->size, msg->data);
        if (ret != ESP_OK) {
          esp3d_log_e("Error writing message");
        }
        deleteMsg(msg);
      }
    }
  }
}

void ESP3DBTSerialClient::flush() {
  uint8_t loopCount = 10;
  while (loopCount && getTxMsgsCount() > 0) {
    loopCount--;
    handle();
    esp3d_hal::wait(50);
  }
}

void ESP3DBTSerialClient::end() {
  if (_started) {
    flush();
    _started = false;
    clearRxQueue();
    clearTxQueue();
    esp3d_hal::wait(1000);
    if (pthread_mutex_destroy(&_tx_mutex) != 0) {
      esp3d_log_w("Mutex destruction for tx failed");
    }
    if (pthread_mutex_destroy(&_rx_mutex) != 0) {
      esp3d_log_w("Mutex destruction for rx failed");
    }
    esp_spp_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
  }
  if (_data) {
    free(_data);
    _data = NULL;
  }
  if (_rxBuffer) {
    free(_rxBuffer);
    _rxBuffer = NULL;
  }
  _rxBufferPos = 0;
}

bool ESP3DBTSerialClient::scan(std::vector<BTDevice>& devices) {
  if (!_started) {
    esp3d_log_e("BT Serial not started");
    return false;
  }

  discovered_devices.clear();
  _last_scan_results.clear();
  esp_err_t ret = esp_bt_gap_register_callback(esp_bt_gap_callback);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to register GAP callback");
    return false;
  }

  ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY,
                                   _config->scan_duration, 0);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to start discovery");
    return false;
  }

  esp3d_hal::wait(_config->scan_duration * 1000 + 1000);
  devices = _last_scan_results = discovered_devices;
  return true;
}

bool ESP3DBTSerialClient::getDeviceAddress(const std::string& name, esp_bd_addr_t& addr) {
  for (const auto& device : _last_scan_results) {
    if (device.name == name) {
      memcpy(addr, device.addr, ESP_BD_ADDR_LEN);
      return true;
    }
  }
  esp3d_log_e("Device %s not found in last scan", name.c_str());
  return false;
}

#endif // ESP3D_BT_FEATURE