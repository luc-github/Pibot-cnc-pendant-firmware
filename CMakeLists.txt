# #######################################
# Select one targeted hardware
# #######################################

# With TFT:
OPTION(ESP32_PIBOT_CNC_PENDANT_V1 "PiBot CNC Pendant V1.0" ON)

# Without TFT:




# ===========================================
# Select one targeted firmware
# ===========================================
OPTION(TARGET_FW_MARLIN "Marlin firmware" OFF)
OPTION(TARGET_FW_REPETIER "Repetier firmware" OFF)
OPTION(TARGET_FW_SMOOTHIEWARE "Smoothieware firmware" OFF)
OPTION(TARGET_FW_GRBL "GRBL firmware" OFF)
OPTION(TARGET_FW_GRBLHAL "grblHAL firmware" ON)
OPTION(TARGET_FW_FLUIDNC "FluidNC firmware" OFF)



# ===========================================
# Select the Features
# ===========================================
OPTION(ESP3D_AUTHENTICATION "Authentication on all clients" OFF)
OPTION(DISABLE_SERIAL_AUTHENTICATION "Disable Serial Authentication" ON)
OPTION(TIME_SERVICE  "Time service" OFF)
OPTION(SSDP_SERVICE "SSDP service" OFF)
OPTION(MDNS_SERVICE "MDNS service" OFF)
OPTION(WIFI_SERVICE "WiFi service" OFF)
OPTION(BT_SERVICE "Bluetooth services" ON)
OPTION(WEB_SERVICES "Web Services http/websocket/webdav/camera" OFF)
OPTION(CAMERA_SERVICE "Camera service" OFF)
OPTION(WEBDAV_SERVICES "WebDav Services" OFF)
OPTION(TELNET_SERVICE "Telnet service" OFF)
OPTION(WS_SERVICE "WebSocket data service" OFF)
OPTION(TFT_UI_SERVICE "TFT UI service" ON)
OPTION(TFT_TOUCH_SERVICE "TFT Touch service" OFF)
OPTION(SD_CARD_SERVICE "SD card service" ON)
OPTION(NOTIFICATIONS_SERVICE "Notifications service" OFF)
OPTION(UPDATE_SERVICE "Update service" OFF)
OPTION(SERIAL_SERVICE "Use Serial if Available" OFF)
OPTION(USB_SERIAL_SERVICE "Use USB Serial if Available" OFF)
OPTION(USE_FAT_INSTEAD_OF_LITTLEFS "Use FAT instead of LittleFS" OFF)

# ===========================================
# Internal Configuration (Do Not Modify)
# ===========================================

set(PLATFORMS_LIST
  ESP32_PIBOT_CNC_PENDANT_V1
)

set(FIRMWARES_LIST
  TARGET_FW_MARLIN
  TARGET_FW_REPETIER
  TARGET_FW_SMOOTHIEWARE
  TARGET_FW_GRBL
  TARGET_FW_GRBLHAL
  TARGET_FW_FLUIDNC
)

cmake_minimum_required(VERSION 3.12.4)
set(CMAKE_CXX_STANDARD 20)

include (cmake/targets.cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

include (cmake/features.cmake)

project(ESP3D-TFT
    VERSION 2.0
    DESCRIPTION "ESP3D TFT")

