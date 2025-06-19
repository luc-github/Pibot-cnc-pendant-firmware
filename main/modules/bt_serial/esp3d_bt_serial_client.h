/*
  esp3d_bt_serial_client

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#pragma once

#if ESP3D_BT_FEATURE

#include <pthread.h>
#include <vector>
#include <string>
#include "esp3d_client.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp3d_bt_serial_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BTDevice {
  esp_bd_addr_t addr;
  std::string name;
  int8_t rssi; 
};

class ESP3DBTSerialClient : public ESP3DClient {
 public:
  ESP3DBTSerialClient();
  ~ESP3DBTSerialClient();
  bool configure(esp3d_bt_serial_config_t* config);
  bool begin();
  void handle();
  void end();
  void process(ESP3DMessage* msg);
  bool isEndChar(uint8_t ch);
  bool pushMsgToRxQueue(const uint8_t* msg, size_t size);
  void flush();
  bool started() { return _started; }
  bool scan(std::vector<BTDevice>& devices);
  bool getDeviceAddress(const std::string& name, esp_bd_addr_t& addr);
  bool disconnect();
  bool connect();
  bool isConnected() { return (_spp_handle != -1); }
  bool clearBondedDevices();
  void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param);
  void sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param);
  char *bda2str(uint8_t * bda, char *str, size_t size);
  bool str2bda(const char *str, esp_bd_addr_t bda);
  bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len);
  uint8_t rssi_to_percentage(int8_t rssi) ;
  const char* getCurrentName() {
    return _current_name.c_str();
  }
  const char* getCurrentAddress() {
    return _current_address.c_str();
  }
 private:
  esp3d_bt_serial_config_t* _config;
  bool _started;
  bool _discovery_started;
  bool _scan_completed;
  pthread_mutex_t _tx_mutex;
  pthread_mutex_t _rx_mutex;
  int _spp_handle;
  std::vector<BTDevice> _last_scan_results;
  uint8_t* _rxBuffer;
  size_t _rxBufferPos;
  std::string _current_name;
  std::string _current_address;
};

extern ESP3DBTSerialClient btSerialClient;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE