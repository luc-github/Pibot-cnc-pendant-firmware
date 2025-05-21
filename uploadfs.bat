@echo off

set IDF_PATH=C:\Users\luc\esp\v5.4.1\esp-idf

python %IDF_PATH%\components\esptool_py\esptool\esptool.py --chip esp32 --port COM17 --baud 921600 write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0x3d0000 build\littlefs.bin

echo LittleFS image flashed successfully!
