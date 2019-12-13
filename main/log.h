#pragma once

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define LOG_FUNC_BEGIN(TAG) ESP_LOGD(TAG, "Entering %s \n", __FUNCTION__);
#define LOG_FUNC_END(TAG) ESP_LOGD(TAG, "Exiting %s \n", __FUNCTION__);
#define LOG(s) printf("%s : %s \n",__FUNCTION__,s);