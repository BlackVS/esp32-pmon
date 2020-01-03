#include "app.h"

static const char *TAG = __FILE__;

static wifi_ap_record_t _wifi_ap_info[DEFAULT_SCAN_LIST_SIZE];
static uint16_t _wifi_ap_number = 0;

CWiFi WiFi;

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
void wifi_station_start(int channel)
{
  ESP_LOGD(__FUNCTION__, "Starting...");

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  channel = MIN(WIFI_MAX_CH, MAX(1,channel));
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  ESP_LOGD(__FUNCTION__, "Started.");
}


///////////////////////////////////////////////////////////////////////////////////
//
//
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",MAC2STR(event->mac), event->aid);
    }
}



///////////////////////////////////////////////////////////////////////////////////
//
//
CWiFi::CWiFi()
{
    mode=WiFi_MODE_NONE;
    channel=1;
    _promiscuous_cb=NULL;
    strcpy(wf_country.cc, "UA");
    wf_country.schan = 1;
    wf_country.nchan = 14;
    wf_country.max_tx_power = 100, //units is 0.25 i.e. 25dBm
    wf_country.policy = WIFI_COUNTRY_POLICY_MANUAL;
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
  
  //tcpip_adapter_init(); - deprecated
  esp_netif_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
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
void CWiFi::set_mode(WiFi_MODES m)
{
  ESP_LOGD(__FUNCTION__, "Starting mode %s", WiFi_MODE_STR[m]);
  
  if(m==mode) {
    ESP_LOGD(__FUNCTION__, "Already in mode %s", WiFi_MODE_STR[m]);
    return;
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
      break;
    case WiFi_MODE_STA:
      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
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
      mode=m;
      break;
  }
  
  if(_events_cb!=NULL && mode == m)
    _events_cb(WiFi_EVENT_MODE_CHANGED, mode);
  
  
  ESP_LOGD(__FUNCTION__, "Started %s", WiFi_MODE_STR[mode]);
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
    set_mode(WiFi_MODE_STA);

    //esp_netif_init();
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    //esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    //assert(sta_netif);
    //wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    _wifi_ap_number = DEFAULT_SCAN_LIST_SIZE;
    memset(_wifi_ap_info, 0, sizeof(_wifi_ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&_wifi_ap_number, _wifi_ap_info));
    ESP_LOGD(TAG, "Total APs scanned = %u", _wifi_ap_number);
    // for (int i = 0; i < _wifi_ap_number; i++) 
    // {
    //   ESP_LOGI(TAG, "SSID \t\t%s", _wifi_ap_info[i].ssid);
    //   ESP_LOGI(TAG, "MAC  \t\t%02x:%02x:%02x:%02x:%02x:%02x", 
    //     _wifi_ap_info[i].bssid[0], _wifi_ap_info[i].bssid[1], _wifi_ap_info[i].bssid[2],
    //     _wifi_ap_info[i].bssid[3], _wifi_ap_info[i].bssid[4], _wifi_ap_info[i].bssid[5]);
    //   ESP_LOGI(TAG, "RSSI \t\t%d", _wifi_ap_info[i].rssi);
      
    //   print_auth_mode(_wifi_ap_info[i].authmode);
    //   if (_wifi_ap_info[i].authmode != WIFI_AUTH_WEP) {
    //       print_cipher_type(_wifi_ap_info[i].pairwise_cipher, _wifi_ap_info[i].group_cipher);
    //   }
    //   ESP_LOGI(TAG, "Channel \t\t%d\n", _wifi_ap_info[i].primary);
    // }
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
