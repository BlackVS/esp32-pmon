#include "app.h"

static const char *TAG = __FILE__;


wl_handle_t  vfs_initialize(const char * storage_name, const char * mount_path)
{
    static wl_handle_t wl_handle;
    esp_vfs_fat_mount_config_t mount_config;
    mount_config.format_if_mount_failed = true;
    mount_config.max_files = 4;
    mount_config.allocation_unit_size = 512; // 0==sector
    esp_err_t err = esp_vfs_fat_spiflash_mount(mount_path, storage_name, &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return 0;
    }
    return wl_handle;
}
