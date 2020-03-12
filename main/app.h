#pragma once

#define CONFIG_STORE_HISTORY 1
//#define CONFIG_JTAG_ENABLED


#define REG_EXTENDED 1
#define REG_ICASE (REG_EXTENDED << 1)

//#define MAX(x, y) (((x) > (y)) ? (x) : (y))
//#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#include <stdio.h>
#include <vector>
//#include <string>
#include <cstring>
#include <stdlib.h>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>

#include "strings.h"


#include "log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_blufi_api.h>

///// netif not supports C++ compiler yet
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "lwip/apps/sntp.h"
#include "lwip/err.h"

#ifdef __cplusplus
}
#endif

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
#include "wifi.h"
#include "console.h"
#include "targets.h"

//tasks
#include "btn.h"
#include "leds.h"
#include "oled.h"

void Delay(int ms);