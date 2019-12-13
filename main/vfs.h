#pragma once

#include "esp_vfs_fat.h"

#define VFS_MOUNT_PATH "/data"
wl_handle_t  vfs_initialize(const char * storage_name, const char * mount_path);
