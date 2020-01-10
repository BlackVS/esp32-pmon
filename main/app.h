#pragma once

#define CONFIG_STORE_HISTORY 1
#define CONFIG_JTAG_ENABLED

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#include <stdio.h>
//#include <string.h>
#include <cstring>
#include <string>
#include <stdlib.h>
#include <vector>
#include <map>
#include <sstream>


#include "log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"


#include "driver/gpio.h"
#include "sdkconfig.h"

#include "badge_config.h"

#include "nvs.h"
#include "vfs.h"
#include "task.h"
#include "console.h"
#include "wifi.h"
#include "targets.h"

//tasks
#include "btn.h"
#include "leds.h"
#include "oled.h"

void Delay(int ms);