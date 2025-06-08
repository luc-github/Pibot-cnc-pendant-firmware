@echo off

set IDF_PATH=C:\Users\luc\esp\v5.4.1\esp-idf

python %IDF_PATH%\components\esptool_py\esptool\esptool.py --chip esp32 --port COM17 read_flash 0x00000 0x400000 dump_flash.bin

echo Full image dump successfully!
