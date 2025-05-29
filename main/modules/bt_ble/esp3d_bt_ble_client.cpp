/*
  esp3d_bt_ble_client

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#if ESP3D_BT_FEATURE

#include "esp3d_bt_ble_client.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_gattc_api.h"
#include "../esp3d_commands.h"
#include "../esp3d_hal.h"
#include "../esp3d_log.h"
#include "../esp3d_settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

ESP3DBTBleClient btBleClient;

bool ESP3DBTBleClient::configure(esp3d_bt_ble_config_t* config) {
  esp3d_log("Configure BT BLE Client");
  if (config) {
    _config = config;
    return true;
  }
  esp3d_log_e("Invalid configuration");
  return false;
}

static std::vector<BLEDevice> discovered_ble_devices;

static void esp_ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  switch (event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
      if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
        BLEDevice device;
        memcpy(device.addr, param->scan_rst.bda, ESP_BD_ADDR_LEN);
        device.name = "";
        if (param->scan_rst.adv_data_len > 0) {
          uint8_t* adv_data = param->scan_rst.ble_adv;
          uint8_t len;
          uint8_t* name_data = esp_ble_resolve_adv_data(adv_data,
                                                       ESP_BLE_AD_TYPE_NAME_CMPL, &len);
          if (name_data && len > 0) {
            device.name = std::string((char*)name_data, len);
          }
        }
        discovered_ble_devices.push_back(device);
        esp3d_log("Found BLE device: %s (%02X:%02X:%02X:%02X:%02X:%02X)",
                  device.name.c_str(),
                  device.addr[0], device.addr[1], device.addr[2],
                  device.addr[3], device.addr[4], device.addr[5]);
      } else if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
        esp3d_log("BLE scan completed");
      }
      break;
    default:
      break;
  }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t* param) {
  switch (event) {
    case ESP_GATTS_REG_EVT:
      esp_ble_gatts_create_service(gatts_if, &btBleClient._config->service_uuid, 1);
      break;
    case ESP_GATTS_CREATE_EVT:
      esp_ble_gatts_add_char(param->create.service_handle,
                             &btBleClient._config->char_uuid,
                             ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                             ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                             NULL, NULL);
      break;
    case ESP_GATTS_ADD_CHAR_EVT:
      btBleClient._gatt_handle = param->add_char.attr_handle;
      esp_ble_gatts_start_service(param->add_char.service_handle);
      break;
    case ESP_GATTS_WRITE_EVT:
      for (size_t i = 0; i < param->write.len; i++) {
        if (btBleClient._rxBufferPos < btBleClient._config->rx_buffer_size) {
          btBleClient._rxBuffer[btBleClient._rxBufferPos++] = param->write.value[i];
        } else {
          esp3d_log_e("Rx buffer overflow");
          btBleClient._rxBufferPos = 0;
          break;
        }
        if (btBleClient.isEndChar(param->write.value[i])) {
          btBleClient.pushMsgToRxQueue(btBleClient._rxBuffer, btBleClient._rxBufferPos);
          btBleClient._rxBufferPos = 0;
        }
      }
      break;
    default:
      break;
  }
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event,
                                        esp_gatt_if_t gattc_if,
                                        esp_ble_gattc_cb_param_t* param) {
  switch (event) {
    case ESP_GATTC_REG_EVT:
      esp3d_log("GATT client registered");
      break;
    case ESP_GATTC_OPEN_EVT:
      if (param->open.status == ESP_GATT_OK) {
        esp3d_log("GATT client connected");
        esp_ble_gattc_search_service(gattc_if, param->open.conn_id, NULL);
      } else {
        esp3d_log_e("GATT client connection failed, status %d", param->open.status);
      }
      break;
    case ESP_GATTC_SEARCH_RES_EVT:
      if (param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 &&
          param->search_res.srvc_id.uuid.uuid.uuid16 == btBleClient._config->service_uuid) {
        esp3d_log("Found target service UUID: 0x%04x", btBleClient._config->service_uuid);
      }
      break;
    case ESP_GATTC_SEARCH_CMPL_EVT:
      esp3d_log("Service discovery completed");
      break;
    default:
      break;
  }
}

ESP3DBTBleClient::ESP3DBTBleClient() {
  _started = false;
  _config = NULL;
  _gatt_handle = 0;
  _rxBuffer = NULL;
  _rxBufferPos = 0;
}

ESP3DBTBleClient::~ESP3DBTBleClient() {
  end();
}

void ESP3DBTBleClient::process(ESP3DMessage* msg) {
  esp3d_log("Processing message for BT BLE");
  if (!msg) {
    esp3d_log_e("Null message");
    return;
  }
  if (!isConnected()) {
    esp3d_log_w("BT BLE not connected, discarding message");
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

bool ESP3DBTBleClient::isEndChar(uint8_t ch) {
  return ((char)ch == '\n');
}

bool ESP3DBTBleClient::connect(esp_bd_addr_t addr) {
  if (!_started) {
    esp3d_log_e("BT BLE not started");
    return false;
  }
  esp_err_t ret = esp_ble_gattc_open(0, addr, BLE_ADDR_TYPE_PUBLIC, true);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to open GATT client connection");
    return false;
  }
  esp3d_log("Initiated GATT client connection");
  return true;
}

bool ESP3DBTBleClient::begin() {
  end();
  configure(&esp3dBTBleConfig);
  if (!_config) {
    esp3d_log_e("BT BLE not configured");
    return false;
  }

  _rxBuffer = (uint8_t*)malloc(_config->rx_buffer_size);
  if (!_rxBuffer) {
    esp3d_log_e("Failed to allocate memory for rx buffer");
    return false;
  }
  _rxBufferPos = 0;

  if (pthread_mutex_init(&_rx_mutex, NULL) != 0) {
    free(_rxBuffer);
    _rxBuffer = NULL;
    esp3d_log_e("Mutex creation for rx failed");
    return false;
  }
  setRxMutex(&_rx_mutex);

  if (pthread_mutex_init(&_tx_mutex, NULL) != 0) {
    free(_rxBuffer);
    _rxBuffer = NULL;
    pthread_mutex_destroy(&_rx_mutex);
    esp3d_log_e("Mutex creation for tx failed");
    return false;
  }
  setTxMutex(&_tx_mutex);

  esp_err_t ret;
  ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to release BT Classic memory");
    return false;
  }

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to init BT controller");
    return false;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
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

  ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to register GATT server callback");
    return false;
  }

  ret = esp_ble_gattc_register_callback(gattc_profile_event_handler);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to register GATT client callback");
    return false;
  }

  ret = esp_ble_gatts_app_register(0);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to register GATT server app");
    return false;
  }

  ret = esp_ble_gattc_app_register(0);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to register GATT client app");
    return false;
  }

  char hostname[32] = "PibotCNC";
  esp3dTftsettings.readString(ESP3DSettingIndex::esp3d_hostname, hostname, sizeof(hostname));
  strncpy(_config->device_name, hostname, sizeof(_config->device_name) - 1);
  _config->device_name[sizeof(_config->device_name) - 1] = '\0';
  esp_ble_gap_set_device_name(_config->device_name);
  esp_ble_gap_config_adv_data_raw(NULL, 0); // À configurer selon vos besoins

  _started = true;
  flush();
  return true;
}

bool ESP3DBTBleClient::pushMsgToRxQueue(const uint8_t* msg, size_t size) {
  ESP3DMessage* newMsgPtr = newMsg();
  if (newMsgPtr) {
    if (ESP3DClient::setDataContent(newMsgPtr, msg, size)) {
      newMsgPtr->origin = ESP3DClientType::bt_ble;
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

void ESP3DBTBleClient::handle() {
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
        esp_ble_gatts_send_response(0, 0, _gatt_handle, msg->size, msg->data, false);
        deleteMsg(msg);
      }
    }
  }
}

void ESP3DBTBleClient::flush() {
  uint8_t loopCount = 10;
  while (loopCount && getTxMsgsCount() > 0) {
    loopCount--;
    handle();
    esp3d_hal::wait(50);
  }
}

void ESP3DBTBleClient::end() {
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
    esp_ble_gatts_app_unregister(0);
    esp_ble_gattc_app_unregister(0);
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
  }
  if (_rxBuffer) {
    free(_rxBuffer);
    _rxBuffer = NULL;
  }
  _rxBufferPos = 0;
}

bool ESP3DBTBleClient::scan(std::vector<BLEDevice>& devices) {
  if (!_started) {
    esp3d_log_e("BT BLE not started");
    return false;
  }

  discovered_ble_devices.clear();
  _last_scan_results.clear();
  esp_err_t ret = esp_ble_gap_register_callback(esp_ble_gap_cb);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to register GAP callback");
    return false;
  }

  esp_ble_scan_params_t scan_params = {
      .scan_type = BLE_SCAN_TYPE_ACTIVE,
      .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
      .scan_interval = 0x50,
      .scan_window = 0x30,
      .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE
  };
  ret = esp_ble_gap_set_scan_params(&scan_params);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to set scan params");
    return false;
  }

  ret = esp_ble_gap_start_scanning(_config->scan_duration);
  if (ret != ESP_OK) {
    esp3d_log_e("Failed to start scanning");
    return false;
  }

  esp3d_hal::wait(_config->scan_duration * 1000 + 1000);
  devices = _last_scan_results = discovered_ble_devices;
  return true;
}

bool ESP3DBTBleClient::getDeviceAddress(const std::string& name, esp_bd_addr_t& addr) {
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