#!/bin/bash

esptool.py -p /dev/ttyUSB0 -b 2000000 read_flash 0x0 0x400000 full_badge_backup_read.bin