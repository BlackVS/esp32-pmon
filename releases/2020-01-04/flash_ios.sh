#!/bin/bash

# To flash all build output, run 'make flash' or:
sudo esptool.py --chip esp32 --port /dev/cu.SLAB_USBtoUART --baud 2000000 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 bootloader.bin 0x10000 nnc-badge-pmon.bin 0x8000 partitions.bin
