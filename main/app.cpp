#include "app.h"

static const char *TAG = __FILE__;

void Delay(int ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void self_test(void *pvParameters) {
  LOG_FUNC_BEGIN(TAG)
  const int delay1 = 10;
  const int delay2 = 50; 
  const int LED_OFF = 1;
  const int LED_ON = 0;

  gpio_set_level(PIN_LED_BLUE, LED_OFF);
  while (1) {
    if (touchpad_get_state()==TOUCHPAD_STATE_OFF)
    {
      vTaskDelay(delay2); //must do task delay to not block threads
      continue;
    }
    gpio_set_level(PIN_LED_BLUE, LED_ON);
    vTaskDelay(delay1);
    gpio_set_level(PIN_LED_BLUE, LED_OFF);
    vTaskDelay(delay1);
    gpio_set_level(PIN_LED_BLUE, LED_ON);
    vTaskDelay(delay1);
    gpio_set_level(PIN_LED_BLUE, LED_OFF);
    vTaskDelay(delay2);
  }
  LOG_FUNC_END(TAG)
  vTaskDelete(NULL);
}

extern "C" {
    void app_main(void);
}

void app_main() 
{
  /* Disable buffering on stdin */
  setvbuf(stdin, NULL, _IONBF, 0);

  nvs_initialize();
  vfs_initialize("storage",VFS_MOUNT_PATH);
  console_init();

//  esp_log_level_set("*", ESP_LOG_NONE);
//esp_log_level_set("*", ESP_LOG_DEBUG);
  esp_log_level_set("*", ESP_LOG_WARN);
  

  printf(LOG_COLOR_W
         "\n\n\nNoNameBadge 2019 created by TechMaker https://techmaker.ua/\n"
         "ESP32 NNC Badge PacketMonitor & Deauth detector by VVS\n\n"
         );

  
  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
          chip_info.cores,
          (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
          (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d, ", chip_info.revision);

  printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
          (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");


  //init board GPIO
  gpio_init(); // all gpio init staff here - to avoid conflicts between models like MPU, OLED etc

  //tasks init - variables etc
  leds_task_init();
  touchpad_task_init(); //JTAG conflict!!!
  oled_task_init(); //i2c init moved to gpio_init

  WiFi.init();

  //Touchpad: reads input
  xTaskCreate(&touchpad_task, "Touchpad", 1024 * 4, NULL, 5, NULL);

  //LEDS task for Alarm
  xTaskCreate(&leds_alarm_task, "LEDS Alarm", 1024 * 4, NULL, 5, NULL);

  oled_draw_logo();
  oled_engine_init();
  
//  printf("Size of wifi_target_t=%i\n", sizeof(wifi_target_t));
//  printf("Size of mac_t=%i\n", sizeof(mac_t));
  //console_task("pmon -c 1 --start");
  //console_task();
  console_task("pmon -c 1 --start", true);//run startup if present
}
