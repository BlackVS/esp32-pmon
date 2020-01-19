#include "app.h"


static const char *TAG = __FILE__;

typedef struct {
  std::string name;
  std::string title;
  std::string author;
  std::string version;
  std::string date;
  std::string source;
  std::string description;
  //
  std::string bin_url;
} NPM_PACKET;

std::vector<NPM_PACKET> packets;

static struct {
    struct arg_lit *list;
    struct arg_lit *verbose;
    struct arg_int *install;
    struct arg_end *end;
} _npm_args;



esp_err_t npm_parse(const std::string& data, NPM_PACKET& res)
{
  ARRSTR text;
  int cnt=strsplit(data, text);
  if(cnt<7){
    printf("Wrong packets info file!\n");
    return ESP_ERR_INVALID_ARG;
  }
  for(auto& s: text){
    //split by :
    size_t pd=s.find(':');
    if(pd==std::string::npos) {
      printf("Wrong packets info file - no delimiter found!\n");
      return ESP_ERR_INVALID_ARG;
    }
    std::string key  =s.substr(0,pd); strtrim(key);
    std::string value=s.substr(pd+1); strtrim(value);
    ESP_LOGD(TAG, "[%s]=[%s]", key.c_str(), value.c_str());
    if(key.compare("NAME")==0){
      res.name=value;
    } else
    if(key.compare("TITLE")==0) {
      res.title=value;
    } else
    if(key.compare("AUTHOR")==0){
      res.author=value;
    } else
    if(key.compare("VERSION")==0){
      res.version=value;
    } else
    if(key.compare("DATE")==0){
      res.date=value;
    } else
    if(key.compare("SOURCE")==0){
      res.source=value;
    } else
    if(key.compare("DESCRIPTION")==0){
      res.description=value;
    } else {
      ESP_LOGD(TAG, "Unknown key %s", key.c_str());
    }
   }
  return ESP_OK;
}

esp_err_t npm_load_list(void)
{
  packets.clear();
  std::string data;
  esp_err_t err=http_get(NPM_REPOSITORY_URL, data);
  if(err!=ESP_OK){
    ESP_LOGE(TAG, "Failed to connect to repository!");
    return err;
  }
  //printf("Read %i bytes\n", (int)data.size());
  ARRSTR purls;
  int pcnt=strsplit(data,purls);
  if(pcnt){
    printf("%i packets available:\n",pcnt);
    for(int i=0;i<pcnt;i++){
      printf("Reading package : %i\n",i);
      std::string purl=purls[i];
      //1. read title
      std::string ptitle=purl+"title.txt";
      err=http_get( ptitle.c_str(), data);
      if(err!=ESP_OK){
        ESP_LOGE(TAG,"Failed to get %s", purl.c_str());
        continue;
      }
      NPM_PACKET pckt;
      err=npm_parse(data, pckt);
      if(err!=ESP_OK) {
        ESP_LOGE(TAG,"Failed to parse packet info");
        continue;
      }
      pckt.bin_url=purl+"bin/firmware.bin";
      packets.push_back(pckt);
    }
  }
  return ESP_OK;
}

void npm_print_list(bool bDetailed=false)
{
  if(packets.size()==0) {
    printf("No packets vailable\n");
  }
  printf("There are %i packets in repository:\n", (int)packets.size());
  if(!bDetailed) {
    printf("------------------------------------------------------------\n");
    printf("ID  Name             Title                                  \n");
    printf("------------------------------------------------------------\n");
  } else {
    printf("------------------------------------------------------------\n");
  }
  for(int i=0;i<packets.size();i++){
    if(bDetailed) {
      printf("ID     : %i\n", i);
      printf("NAME   : %s\n", packets[i].name.c_str());
      printf("TITLE  : %s\n", packets[i].title.c_str());
      printf("AUTHOR : %s\n", packets[i].author.c_str());
      printf("VERSION: %s\n", packets[i].version.c_str());
      printf("DATE   : %s\n", packets[i].date.c_str());
      printf("SOURCE : %s\n", packets[i].source.c_str());
      printf("BIN_URL: %s\n", packets[i].bin_url.c_str());
      printf("DESCRIPTION:  %s\n", packets[i].description.c_str());
      printf("------------------------------------------------------------\n");
    } else {
      printf("%2i  %-16s %s\n",i, packets[i].name.c_str(), packets[i].title.c_str());
    }
  }
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static size_t _npm_content_length=0;
esp_err_t _http_npm_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            //printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
            if(strcmp(evt->header_key,"Content-Length")==0){
              _npm_content_length=atoi(evt->header_value);
              //printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
              printf("Firmware size: %i\n", _npm_content_length);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                ESP_LOGD(TAG, "data: %.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            // {
            //   ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            //   int mbedtls_err = 0;
            //   esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            //   if (err != 0) {
            //       ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            //       ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            //   }
            // }
            break;
    }
    return ESP_OK;
}

esp_err_t npm_packet_load(int id)
{
    if(id>=packets.size()){
      printf("Error: wrong id!\n");
      return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));
    config.url=packets[id].bin_url.c_str();
    config.skip_cert_common_name_check = true;
    config.event_handler=_http_npm_event_handler;
    // esp_err_t ret = esp_https_ota(&config);
    // if (ret == ESP_OK) {
    //     esp_restart();
    // } else {
    //     ESP_LOGE(TAG, "Firmware upgrade failed");
    // }
    // while (1) {
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    esp_https_ota_config_t ota_config;
    ota_config.http_config = &config;
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        esp_https_ota_finish(https_ota_handle);
        return err;
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        esp_https_ota_finish(https_ota_handle);
        return err;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "image header verification failed");
        esp_https_ota_finish(https_ota_handle);
        return err;
    }

    size_t len_read=0;
    int    progress=0;
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        len_read=esp_https_ota_get_image_len_read(https_ota_handle);
        //ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
        if(_npm_content_length) {
          int p= (len_read*100)/_npm_content_length;
          if(p-progress>=5){
            progress=p;
            printf("Progress: %i%%\n",progress);
          }
        }
    }

    // if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
    //     // the OTA image was not completely received and user can customise the response to this situation.
    //     ESP_LOGE(TAG, "Complete data was not received.");
    // }

    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    } else {
        ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed %d", ota_finish_err);
    }

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    return ESP_OK;
}


/* 'version' command */
int cmd_npm(int argc, char **argv) 
{
  int nerrors = arg_parse(argc, argv, (void **)&_npm_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, _npm_args.end, argv[0]);
    return 0;
  }

  bool bList   =_npm_args.list->count > 0;
  bool bInstall=_npm_args.install->count > 0;
  if(!bList&&!bInstall)
    bList=true;
  if(bList&&bInstall){
    printf("Please use only one command at the same time!\n");
    return 0;
  }

  if(!wifi_join.is_connected()){
    printf("Please connect to Internet first!\n");
    return 0;
  }
  
  if(bList) {
    if(npm_load_list()==ESP_OK){
      npm_print_list(_npm_args.verbose->count > 0);
    } else {
      printf("Failed to load packets list!!!\n");
    }
  }

  if(bInstall){
    esp_err_t err=npm_packet_load(_npm_args.install->ival[0]);
    if(err!=ESP_OK) {
      printf("Failed to upload new firmware!\n");
    } else {
      printf("Ok!");
    }
  }
  return 0;
}

void register_cmd_npm() {
  _npm_args.list = arg_lit0("l", "list", "List available packets");
  _npm_args.verbose = arg_lit0("v", "verbose", "Verbose/detailed output");
  _npm_args.install = arg_int0("i", "install", "<packet_id", "Install packet");
  _npm_args.end = arg_end(1);

  const esp_console_cmd_t cmd = {
      .command = "npm",
      .help = "NoName badge Packets Manager",
      .hint = NULL,
      .func = &cmd_npm,
      .argtable = &_npm_args
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
