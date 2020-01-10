#include "app.h"

void nvs_initialize(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void nvs_settings_setU32(const char *name, uint32_t value) {
  nvs_handle my_handle;
  nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  nvs_set_u32(my_handle, name, value);
  nvs_commit(my_handle);
  nvs_close(my_handle);
}

uint32_t nvs_settings_getU32(const char *name) {
  uint32_t value = 0;

  nvs_handle my_handle;
  nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  nvs_get_u32(my_handle, name, &value);
  nvs_close(my_handle);

  return value;
}

void nvs_settings_setI32(const char *name, int32_t value) {
  nvs_handle my_handle;
  nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  nvs_set_i32(my_handle, name, value);
  nvs_commit(my_handle);
  nvs_close(my_handle);
}

int32_t nvs_settings_getI32(const char *name) {
  int32_t value = 0;

  nvs_handle my_handle;
  nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  nvs_get_i32(my_handle, name, &value);
  nvs_close(my_handle);

  return value;
}

esp_err_t nvs_settings_setString(const char *name, const char *value) {
  esp_err_t err=ESP_OK;
  nvs_handle my_handle;
  nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  err=nvs_set_str(my_handle, name, value);
  nvs_commit(my_handle);
  nvs_close(my_handle);
  return err;
}

esp_err_t nvs_settings_setString(const char *name, std::string  value) 
{
  return nvs_settings_setString(name, value.c_str());
}

esp_err_t nvs_settings_getString(const char *name, char *value, size_t maxLen) {
  esp_err_t err=ESP_OK;
  nvs_handle my_handle;
  nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  err=nvs_get_str(my_handle, name, value, &maxLen);
  nvs_close(my_handle); 
  return err;
}

esp_err_t nvs_settings_getString(const char *name, std::string& value) {
  esp_err_t err=ESP_OK;
  nvs_handle my_handle;
  nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  size_t l=0;  
  err=nvs_get_str(my_handle, name, NULL, &l);
  if(err==ESP_OK&&l>0) {
    value.resize(l,0);
    l=value.size();
    err=nvs_get_str(my_handle, name, &value[0], &l);
  }
  nvs_close(my_handle);
  return err;
}