/*
  esp3d_bt_ble_client

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
#pragma once

#if ESP3D_BT_FEATURE

#include <pthread.h>
#include <vector>
#include "../esp3d_client.h"
#include "../esp3d_bt_ble_config.h"
#include "esp_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BLEDevice {
  esp_bd_addr_t addr;  // Adresse MAC
  std::string name;    // Nom de l'appareil
};

class ESP3DBTBleClient : public ESP3DClient {
 public:
  ESP3DBTBleClient();
  ~ESP3DBTBleClient();
  bool configure(esp3d_bt_config_t* config);
  bool begin();
  void handle();
  void end();
  void process(ESP3DMessage* msg);
  bool isEndChar(uint8_t ch);
  bool pushMsgToRxQueue(const uint8_t* msg, size_t size);
  void flush();
  bool started() { return _started; }
  void readBLE();
  bool scan(std::vector<BLEDevice>& devices);
  bool getDeviceAddress(const std::string& name, esp_bd_addr_t& addr);
  bool connect(esp_bd_addr_t addr);  // Connexion client GATT

 private:
  esp3d_bt_config_t* _config;
  TaskHandle_t _xHandle;
  bool _started;
  pthread_mutex_t _tx_mutex;
  pthread_mutex_t _rx_mutex;
  uint8_t* _data;
  uint8_t* _buffer;
  size_t _bufferPos;
  uint16_t _gatt_handle;
  std::vector<BLEDevice> _last_scan_results;
};

extern ESP3DBTBleClient btBleClient;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ESP3D_BT_FEATURE