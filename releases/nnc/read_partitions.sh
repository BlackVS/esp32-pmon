#!/bin/bash

esptool.py --chip esp32 --port /dev/ttyUSB0 read_flash 0x8000 0xc00 ptable.img
gen_esp32part.py ptable.img > ptable.csv

esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0x1000 0x7000 bin/bootloader.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0x8000 0xc00 bin/partitions.bin
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0x9000 0x4000 bin/nvs.bin
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0xd000 0x2000 bin/ota_data_initial.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0xf000 0x1000 bin/phy.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0x10000 0x180000 bin/ota_0.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0x19000 0x180000 bin/ota_1.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0x310000 0x1000 bin/nvs_key.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 2000000 read_flash 0x311000 0x4000 bin/nnc_data.bin 

