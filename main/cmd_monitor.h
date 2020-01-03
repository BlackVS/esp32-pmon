#pragma once

#define CONFIG_PMON_TASK_STACK_SIZE 1024*8
#define CONFIG_PMON_TASK_PRIORITY 5

#define PACKET_PROCESS_PACKET_TIMEOUT_MS (100)


///// netif not supports C++ compiler yet
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"
#include "lwip/apps/sntp.h"
#include "lwip/err.h"

#ifdef __cplusplus
}
#endif

// 1 - 14 channels (1-11 for US, 1-13 for EU and 1-14 for Japan)
#define WIFI_MAX_CH 14      
#define WIFI_SNAP_LEN 2324   // max len of each recieved packet






void register_cmd_monitor(void);

