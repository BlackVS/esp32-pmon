#include "app.h"

/* 'version' command */
int cmd_version(int argc, char **argv) 
{
  esp_chip_info_t info;
  esp_chip_info(&info);

  char data[256] = {0};

  printf("Firmware Version: %s\r\n", NNC_FW_VERSION);
  nvs_settings_getString(DSA_HASH1, data, 256);
  printf("Firmware Digest: %s\r\n", data);
  
  nvs_settings_getString(DSA_SIGN1, data, 256);
  printf("Firmware Signature: %s\r\n", data);
  
  nvs_settings_getString(DSA_HASH2, data, 256);
  printf("Partition Table Digest: %s\r\n", data);
  
  nvs_settings_getString(DSA_SIGN2, data, 256);
  printf("Partition Table Signature: %s\r\n", data);

  //printf("Bootloader PubKey:\r\n%s\r\n", (char *)pubkey_start);

  printf("Chip info:\r\n");
  printf("\tmodel:%s\r\n", info.model == CHIP_ESP32 ? "ESP32" : "Unknown");
  printf("\tcores:%d\r\n", info.cores);
  printf("\tfeature:%s%s%s%s%d%s\r\n",
         info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
         info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
         info.features & CHIP_FEATURE_BT ? "/BT" : "",
         info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:"
                                                : "/External-Flash:",
         spi_flash_get_chip_size() / (1024 * 1024), " MB");
  printf("\trevision number:%d\r\n", info.revision);
  return 0;
}

void register_cmd_version() {
  const esp_console_cmd_t cmd = {
      .command = "version",
      .help = "Get version of chip and SDK",
      .hint = NULL,
      .func = &cmd_version,
      .argtable = NULL
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int do_reboot(int argc, char **argv) 
{ 
  esp_restart(); 
}


static void register_cmd_reboot() {
  const esp_console_cmd_t cmd = {
      .command = "reboot",
      .help = "Software reset of the chip",
      .hint = NULL,
      .func = &do_reboot,
      .argtable = NULL
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_cmd_system(void)
{
  register_cmd_version();
  register_cmd_reboot();
  register_cmd_startup();
}