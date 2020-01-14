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
#include "cmd_system.h"
#include "cmd_monitor.h"
#include "cmd_tester.h"
#include "cmd_scan.h"
#include "cmd_targets.h"
#include "cmd_join.h"
#include "cmd_npm.h"
#include "cmd_startup.h"
#include "cmd_radar.h"


#define CONSOLE_HISTORY_PATH "/data/history.txt"


void console_task(const char *startcmd=NULL, bool bStartupEnable=false);

void console_init(void);

esp_err_t console_script_run(const char * filename);