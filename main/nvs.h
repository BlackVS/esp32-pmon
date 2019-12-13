#pragma once

#include "nvs.h"
#include "nvs_flash.h"

#define NVS_NAMESPACE "nnc_data"

#define DSA_HASH1 "dsa-hash1"
#define DSA_SIGN1 "dsa-sign1"
#define DSA_HASH2 "dsa-hash2"
#define DSA_SIGN2 "dsa-sign2"


void        nvs_initialize(void);
void        nvs_settings_setU32(const char *name, uint32_t value);
uint32_t    nvs_settings_getU32(const char *name);
void        nvs_settings_setI32(const char *name, int32_t value);
int32_t     nvs_settings_getI32(const char *name);
void        nvs_settings_setString(const char *name, const char *value);
void        nvs_settings_getString(const char *name, char *value, size_t maxLen);