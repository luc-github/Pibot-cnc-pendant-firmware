/*
  esp3d_bt_ble_client

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#pragma once

#if ESP3D_BT_FEATURE

#include <pthread.h>
#include <vector>
#include "esp3d_client.h"
#include "esp_bt.h"
#include "esp3d_bt_ble_config.h"
#include <string>


#ifdef __cplusplus
extern "C" {
#endif

struct BLEDevice {
  esp_bd_addr_t addr;
  std::string name;
};

class ESP3DBTBleClient : public ESP3DClient {
 public:
  ESP3DBTBleClient();
  ~ESP3DBTBleClient();
  bool configure(esp3d_bt_ble_config_t* config);
  bool begin();
  void handle();
  void end();
  void process(ESP3DMessage* msg);
  bool isEndChar(uint8_t ch);
  bool pushMsgToRxQueue(const uint8_t* msg, size_t size);
  void flush();
  bool started() { return _started; }
  bool scan(std::vector<BLEDevice>& devices);
  bool getDeviceAddress(const std::string& name, esp_bd_addr_t& addr);
  bool connect(esp_bd_addr_t addr);
  bool isConnected() { return _gatt_handle != 0; }
  //void esp_ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);

 private:
  esp3d_bt_ble_config_t* _config;
  bool _started;
  pthread_mutex_t _tx_mutex;
  pthread_mutex_t _rx_mutex;
  uint16_t _gatt_handle;
  std::vector<BLEDevice> _last_scan_results;
  uint8_t* _rxBuffer;
  size_t _rxBufferPos;
};

extern ESP3DBTBleClient btBleClient;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE