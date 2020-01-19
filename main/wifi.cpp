#include "app.h"

static const char *TAG = __FILE__;

static wifi_ap_record_t _wifi_ap_info[DEFAULT_SCAN_LIST_SIZE];
static uint16_t _wifi_ap_number = 0;

CWiFi WiFi;

CTaskPool pool_wifi_tasks(true);

const char* WIFI_AUTH_MODE_STR[WIFI_AUTH_MAX] = {
          "WIFI_AUTH_OPEN",
          "WIFI_AUTH_WEP",
          "WIFI_AUTH_WPA_PSK",
          "WIFI_AUTH_WPA2_PSK",
          "WIFI_AUTH_WPA_WPA2_PSK",
          "WIFI_AUTH_WPA2_ENTERPRISE"
};

const char* WIFI_CIPHER_TYPE_STR[WIFI_CIPHER_TYPE_UNKNOWN] = {
        "WIFI_CIPHER_TYPE_NONE",
        "WIFI_CIPHER_TYPE_WEP40",
        "WIFI_CIPHER_TYPE_WEP104",
        "WIFI_CIPHER_TYPE_TKIP",
        "WIFI_CIPHER_TYPE_CCMP",
        "WIFI_CIPHER_TYPE_TKIP_CCMP"
};

static const char * WiFi_MODE_STR[WiFi_MODE_MAX] = {
  "None",
  "STA",
  "AP",
  "APSTA",
  "PROMISCUOUS"
};

const char* authmode2str(wifi_auth_mode_t authmode)
{
    if(authmode<WIFI_AUTH_MAX)
      return WIFI_AUTH_MODE_STR[authmode];
    return "WIFI_AUTH_UNKNOWN";
}

const char* cipher2str(wifi_cipher_type_t cipher)
{
  if(cipher<WIFI_CIPHER_TYPE_UNKNOWN)
    return WIFI_CIPHER_TYPE_STR[cipher];
  return "WIFI_CIPHER_TYPE_UNKNOWN";
}

std::string mac2str(mac_t mac)
{
  char buf[32]={0};
  mac2str_n(buf,sizeof(buf)-1,mac);
  return buf;
}

uint32_t mac2str_n(char* buf, uint32_t bufsize, mac_t mac)
{
  return snprintf(buf, bufsize-1, "%02x:%02x:%02x:%02x:%02x:%02x", mac.addr[0], mac.addr[1], mac.addr[2], mac.addr[3], mac.addr[4], mac.addr[5]);
}

///////////////////////////////////////////////////////////////////////////////////
//
//
esp_err_t wifi_event_handler(void* ctx, system_event_t *event)
{
    switch(event->event_id)
    {
      case SYSTEM_EVENT_STA_START:
        {
          if(WiFi._join2ap_state==WiFi_JOIN2AP_CONNECTING)
          {
            esp_err_t err=esp_wifi_connect();
            if(err!=ESP_OK){ 
              //ESP_ERR_WIFI_NOT_STARTED
              ESP_LOGW(TAG,"Failed to connect, err=%x", err);
              WiFi._join2ap_state=WiFi_JOIN2AP_FAILED;
              xEventGroupSetBits(WiFi.wifi_event_group, WiFi_JOIN2AP_BIT_FAILED);
            }
          }
        }
        break;

      case SYSTEM_EVENT_STA_DISCONNECTED:
        if(WiFi._join2ap_state==WiFi_JOIN2AP_CONNECTING)
        {
          if (WiFi._join2ap_retries) {
              esp_wifi_connect();
              WiFi._join2ap_retries--;
              ESP_LOGI(TAG, "Retry to connect to the AP");
          } else {
              WiFi._join2ap_state=WiFi_JOIN2AP_FAILED;
              xEventGroupSetBits(WiFi.wifi_event_group, WiFi_JOIN2AP_BIT_FAILED);
          }
        }
        if(WiFi._join2ap_state==WiFi_JOIN2AP_CONNECTED)
        {
          WiFi._join2ap_state=WiFi_JOIN2AP_IDLE;
          xEventGroupClearBits(WiFi.wifi_event_group, WiFi_JOIN2AP_BITS_ALL);
        }
        break;

      case SYSTEM_EVENT_STA_CONNECTED:
        {
          ESP_LOGI(TAG, "station joined to SSID=%s",(char*)&event->event_info.connected.ssid);
        }
        break;

      // case WIFI_EVENT_AP_STACONNECTED:
      //   {
      //     wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
      //     ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",MAC2STR(event->mac), event->aid);
      //   }
      //   break;

       case SYSTEM_EVENT_STA_GOT_IP:
        if(WiFi._join2ap_state==WiFi_JOIN2AP_CONNECTING)
        {
          ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
          WiFi._join2ap_state=WiFi_JOIN2AP_CONNECTED;
          xEventGroupSetBits(WiFi.wifi_event_group, WiFi_JOIN2AP_BIT_CONNECTED);
        }
        break;

      default:
        break;
   }
   return ESP_OK;
}


///////////////////////////////////////////////////////////////////////////////////
//
//
CWiFi::CWiFi()
{
    mode=WiFi_MODE_NONE;
    channel=1;
    _promiscuous_cb=NULL;
    _join2ap_state=WiFi_JOIN2AP_IDLE;
    _join2ap_retries=5;
    strcpy(wf_country.cc, "UA");
    wf_country.schan = 1;
    wf_country.nchan = 14;
    wf_country.max_tx_power = 100, //units is 0.25 i.e. 25dBm
    wf_country.policy = WIFI_COUNTRY_POLICY_MANUAL;
    wifi_event_group = xEventGroupCreate();
}

///////////////////////////////////////////////////////////////////////////////////
//
//
void CWiFi::init(void)
{
  ESP_LOGD(__FUNCTION__, "Starting...");
  static bool initialized = false;
  if (initialized) {
    return;
  }
  
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));


  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));//??
  ESP_ERROR_CHECK(esp_wifi_set_country(&wf_country));
  //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

  //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  //ESP_ERROR_CHECK(esp_wifi_start()); // - deprecated? Must be called only if mode not null?

  ESP_LOGD(__FUNCTION__, "Started.");
  initialized = true;
}


///////////////////////////////////////////////////////////////////////////////////
//
//
esp_err_t CWiFi::set_mode(WiFi_MODES m, wifi_bandwidth_t bandwidth, wifi_config_t *cfg)
{
  ESP_LOGD(__FUNCTION__, "Starting mode %s", WiFi_MODE_STR[m]);
  
  if(m==mode) {
    ESP_LOGD(__FUNCTION__, "Already in mode %s", WiFi_MODE_STR[m]);
    return ESP_ERR_INVALID_ARG;
  }

  if(m!=WiFi_MODE_NONE && mode!=WiFi_MODE_NONE && m!=mode) //change from active to active mode - first stop old
  {
    ESP_LOGD(__FUNCTION__, "First off previous mode %s", WiFi_MODE_STR[mode]);
    set_mode(WiFi_MODE_NONE);
  }
  
  if(_events_cb!=NULL)
    _events_cb(WiFi_EVENT_MODE_CHANGING, m);

  switch(m){
    case WiFi_MODE_AP:
      if(cfg==NULL){
        ESP_LOGE(TAG,"No config supplied!!!");
        return ESP_ERR_INVALID_ARG;
      }
      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
      ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, cfg));
      ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, bandwidth));
      ESP_ERROR_CHECK(esp_wifi_start());
      mode=m;
      break;
    case WiFi_MODE_STA:
      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
      if(cfg) {
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, cfg));
      }
      ESP_ERROR_CHECK(esp_wifi_start());
      mode=m;
      break;
    case WiFi_MODE_APSTA:
      break;
    case WiFi_MODE_PROMISCUOUS:
      {
        ESP_LOGD(__FUNCTION__, "Enabling promiscuous mode");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(_promiscuous_cb);
        wifi_promiscuous_filter_t filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL
        };
        esp_wifi_set_promiscuous_filter(&filter);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        mode=m;
      }          
      break;
    case WiFi_MODE_MAX:
      break; //just to suppres warning
    ////
    case WiFi_MODE_NONE:
      if(mode==WiFi_MODE_PROMISCUOUS)
      {
        ESP_LOGD(__FUNCTION__, "Shutdown promiscuous mode");
        esp_wifi_set_promiscuous(false);
      }
      ESP_LOGD(__FUNCTION__, "Stop wifi");
      ESP_ERROR_CHECK(esp_wifi_stop());
      if(mode==WiFi_MODE_STA)
      {
        ESP_LOGD(__FUNCTION__, "Stop STA");
        if(_join2ap_state!=WiFi_JOIN2AP_IDLE)
          esp_wifi_disconnect();
        _join2ap_state=WiFi_JOIN2AP_IDLE;
      }
      mode=m;
      break;
  }
  
  if(_events_cb!=NULL && mode == m)
    _events_cb(WiFi_EVENT_MODE_CHANGED, mode);
  
  
  ESP_LOGD(__FUNCTION__, "Started %s", WiFi_MODE_STR[mode]);
  return ESP_OK;
}



///////////////////////////////////////////////////////////////////////////////////
//
//
void CWiFi::set_channel(uint32_t ch, bool bForce)
{
  if(ch==channel)
    return;
   
  channel=ch;
  while(channel>WIFI_MAX_CH)
    channel-=WIFI_MAX_CH;
  if(bForce)
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);  
}


///////////////////////////////////////////////////////////////////////////////////
//
//
uint32_t CWiFi::get_channel(void)
{
  return channel;
}

///////////////////////////////////////////////////////////////////////////////////
//
//
void CWiFi::set_promiscuous_callback(WiFi_PROMISCUOUS_CALLBACK callback)
{
  _promiscuous_cb=callback;
}


///////////////////////////////////////////////////////////////////////////////////
//
//
uint32_t CWiFi::scan_APs(void)
{
    wifi_config_t wifi_config;
    memset(&wifi_config,0,sizeof(wifi_config));
    set_mode(WiFi_MODE_STA, WIFI_BW_HT40, &wifi_config);

    _wifi_ap_number = DEFAULT_SCAN_LIST_SIZE;
    memset(_wifi_ap_info, 0, sizeof(_wifi_ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&_wifi_ap_number, _wifi_ap_info));
    ESP_LOGD(TAG, "Total APs scanned = %u", _wifi_ap_number);
    return _wifi_ap_number;
}


const char * CWiFi::mode2str(WiFi_MODES mode)
{
  return WiFi_MODE_STR[mode];
}

void CWiFi::set_event_handler(WiFi_EVENTS_CALLBACK cb)
{
  if(cb==_events_cb)
    return;
  if(_events_cb!=NULL)
    _events_cb(WiFi_EVENT_UNSUBSCRIBED, 0);
  _events_cb=cb;
  if(_events_cb!=NULL)
    _events_cb(WiFi_EVENT_SUBSCRIBED, 0);
}


int CWiFi::scan_APs_get_count(void)
{
  return _wifi_ap_number;
}

bool CWiFi::scan_APs_get_data(uint32_t idx, wifi_ap_record_t& ap)
{
  if(idx>=_wifi_ap_number)
    return false;
  ap=_wifi_ap_info[idx];
  return true;
}

mac_t CWiFi::get_mac(wifi_interface_t ifx)
{
  uint8_t mac[6];
  esp_wifi_get_mac(ifx,mac);
  return mac_t(mac);
}


esp_err_t CWiFi::join2AP(const char* ssid, const char* pass, int timeout_ms, bool bWait)
{
  _join2ap_state=WiFi_JOIN2AP_CONNECTING;
  _join2ap_retries=DEFAULT_ESP_WIFI_STA_MAXIMUM_RETRY;
  xEventGroupClearBits(WiFi.wifi_event_group, WiFi_JOIN2AP_BITS_ALL);

  wifi_config_t wifi_config;
  memset(&wifi_config,0,sizeof(wifi_config));
  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  if (pass) {
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
  }

  WiFi.set_mode(WiFi_MODE_STA, WIFI_BW_HT40, &wifi_config);

  if(!bWait)
    return ESP_OK;

  int bits = xEventGroupWaitBits(wifi_event_group, WiFi_JOIN2AP_BIT_CONNECTED | WiFi_JOIN2AP_BIT_FAILED, 
                                 false,  //do not reset bits
                                 false,  //wait for any event
                                 timeout_ms / portTICK_PERIOD_MS);

  if(bits & WiFi_JOIN2AP_BIT_CONNECTED){
    printf("WiFi Connected\n");
    return ESP_OK;
  }

  return ESP_FAIL;

}


std::string _httpclient_data;
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                //ESP_LOGD(TAG, "data: %.*s", evt->data_len, (char*)evt->data);
                _httpclient_data.append((char*)evt->data, evt->data_len);
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

esp_err_t http_get(const char *url, std::string& data) 
{
  data.clear();
  ESP_LOGD(TAG, "Reading %s", url);
  esp_http_client_config_t config;
  memset(&config,0,sizeof(config));
  config.url = url;
  config.event_handler = _http_event_handler;
  config.skip_cert_common_name_check = true;
  esp_http_client_handle_t client = esp_http_client_init(&config);
  
  // GET
  _httpclient_data.clear();
  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) 
  {
    int len = esp_http_client_get_content_length(client);
    int statuscode=esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", statuscode, len);
    ESP_LOGD(TAG, "HTTP GET : size %d bytes", _httpclient_data.size());
    ESP_LOGD(TAG, "data=%s",_httpclient_data.c_str());
  } else {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  data=_httpclient_data;
  return err;
}
