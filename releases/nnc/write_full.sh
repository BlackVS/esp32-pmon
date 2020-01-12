#!/bin/bash

esptool.py -p /dev/ttyUSB0 -b 2000000 write_flash 0x0 full_badge_backup.bin