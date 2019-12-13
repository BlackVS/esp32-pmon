#pragma once

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

//#include "cmd_decl.h"
#include "cmd_version.h"
#include "cmd_monitor.h"
#include "cmd_deauth.h"
#include "cmd_scan.h"


#define CONSOLE_HISTORY_PATH "/data/history.txt"


void console_task(const char *startcmd=NULL);

void console_init(void);