/*
  esp3d_bt_serial_client

  Copyright (c) 2025 Luc Lebosse. All rights reserved.
*/
#pragma once

#if ESP3D_BT_FEATURE

#include <pthread.h>
#include <vector>
#include "../esp3d_client.h"
#include "../bt_services_def.h"
#include "esp_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BTDevice {
  esp_bd_addr_t addr;
  std::string name;
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
  bool connect(esp_bd_addr_t addr);
  bool isConnected() { return _spp_handle != -1; }

 private:
  esp3d_bt_serial_config_t* _config;
  bool _started;
  pthread_mutex_t _tx_mutex;
  pthread_mutex_t _rx_mutex;
  uint8_t* _data;
  int _spp_handle;
  std::vector<BTDevice> _last_scan_results;
  uint8_t* _rxBuffer;
  size_t _rxBufferPos;
};

extern ESP3DBTSerialClient btSerialClient;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE