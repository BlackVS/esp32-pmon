#pragma once

#define CONFIG_STORE_HISTORY 1
#define CONFIG_JTAG_ENABLED

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "badge_config.h"

#include "nvs.h"
#include "vfs.h"
#include "console.h"
#include "wifi.h"

//tasks
#include "btn.h"
#include "leds.h"
#include "oled.h"

void Delay(int ms);