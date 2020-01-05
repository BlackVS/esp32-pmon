#include "app.h"

static const char *TAG = __FILE__;

CTesterTask dtester("DTester", 4096, 5, &pool_wifi_tasks);

// https://tools.ietf.org/html/rfc5342#page-4
// https://honeywellaidc.force.com/supportppr/s/article/Locally-Administered-MAC-addresses

/*
Two bits within the initial 3 octets of an EUI-48 have special
   significance: the Group bit (01-00-00) and the Local bit (02-00-00).
...
For globally unique EUI-48 identifiers allocated by an OUI or IAB
   owner, the Local bit is zero.  If the Local bit is a one, the
   identifier is considered by IEEE 802 to be a local identifier under
   the control of the local network administrator.  If the Local bit is
   on, the holder of an OUI (or IAB) has no special authority over
   48-bit MAC identifiers whose first 3 (or 4 1/2) octets correspond to
   their OUI (or IAB).
*/

/*
Address Prefix	3C:71:BF
Vendor / Company	Espressif Inc.
Start Address	3C71BF000000
End Address	3C71BFFFFFFF
*/

/*Address Prefix	80:7D:3A
Vendor / Company	Espressif Inc.
Start Address	807D3A000000
End Address	807D3AFFFFFF*/


static struct {
    struct arg_lit *test;
    struct arg_int *channel;
    #ifdef DTESTER_ARG_PACKETS
    struct arg_int *packets;
    #endif
    struct arg_end *end;
} _dmon_args;

// Add linker flags (component.mk and/or CMakeLists and/or Makefile)
// COMPONENT_ADD_LDFLAGS += -z muldefs
// Redefine ieee80211_raw_frame_sanity_check somewhere
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  ESP_LOGD(TAG,"Intercepted ieee80211_raw_frame_sanity_check");
  return 0;
}


///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_tester_cmd(int argc, char **argv)
{
    ESP_LOGD(__FUNCTION__, "Enter...");
    int nerrors = arg_parse(argc, argv, (void **)&_dmon_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, _dmon_args.end, argv[0]);
        return 0;
    }

    // --channel
    uint32_t channel = 0;//all
    if (_dmon_args.channel->count) {
        channel = _dmon_args.channel->ival[0];
    }
    dtester.channel=channel;

    // --packets
    uint32_t packets = DTESTER_PACKETS_DEFAULT;//use default value
    #ifdef DTESTER_ARG_PACKETS
    if (_dmon_args.packets->count) {
            packets = _dmon_args.packets->ival[0];
    }
    #endif
    dtester.packets=packets;

    //bool bChangeOnTheFly = _deauth_rt.is_running;
    // --test
    if (_dmon_args.test->count) {
        dtester.start();
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
    _dmon_args.test  = arg_lit0("r", "run", "run Deauth detector tester");
    _dmon_args.channel = arg_int1("c", "channel", "<channel>", "WiFi channel to test");
    #ifdef DTESTER_ARG_PACKETS
    _dmon_args.packets = arg_int0("n", "packets", "<packets", "How many packets to send. By default " STR(DTESTER_PACKETS_DEFAULT) " packets");
    #endif
    _dmon_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
        .command = "test",
        .help = "Test deauth detector",
        .hint = NULL,
        .func = &do_tester_cmd,
        .argtable = &_dmon_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGD(__FUNCTION__, "Started.");
}



esp_err_t CTesterTask::init(void) 
{ 
    wifi_config_t ap_config;
    strcpy((char*)ap_config.ap.ssid, "DEAUTH_DETECTER_TESTER");
    ap_config.ap.ssid_len = 0;
    strcpy((char*)ap_config.ap.password, "dummypassword");
    ap_config.ap.channel = channel;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.ssid_hidden = 0;
    ap_config.ap.max_connection = 4;
    if (strlen((char*)ap_config.ap.password) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    WiFi.set_mode(WiFi_MODE_AP, WIFI_BW_HT20, &ap_config);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             ap_config.ap.ssid, 
             ap_config.ap.password);
    return ESP_OK;
}

static uint8_t deauth_frame[] = {
    0xc0, 0x00,                             // 0 ..1 : Frame control (deauth code: 12)
    0x00, 0x00,                             // 2 ..3 : Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,     // 4 ..9 : Destination (broadcast)
    0xe8, 0x94, 0xf6, 0xb5, 0x84, 0xdc,     // 10..15: Transmitter/Source (router)
    0xe8, 0x94, 0xf6, 0xb5, 0x84, 0xdc,     // 16..21: BSSID
    0x00, 0x00,                             // 22..23: Fragment and Sequence number
    0x07, 0x00                              // 24..25: Reason code: Class 3 frame received from nonassociated STA
};

esp_err_t CTesterTask::starting(void) 
{
    leds_alarm_set(true, LED_BLUE, 0.7f);
    mac_t mac = WiFi.get_mac(); //get AP mac
    printf("AP MAC is : %s \n", mac2str(mac).c_str() );
    mac.addr[0] |=  0x02;//local bit set
    mac.addr[0] &= ~0x01;//group bit clear
    printf("AP test MAC is : %s \n", mac2str(mac).c_str() );
    //make it local
    memcpy( deauth_frame+10, mac.addr, 6);
    memcpy( deauth_frame+16, mac.addr, 6);
    return ESP_OK;
}

bool CTesterTask::execute(void) 
{
    uint32_t cnt MIN(2, packets);
    for (uint32_t k = 0; k < cnt; k++) 
    {
        esp_err_t __err_rc = esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
        if (__err_rc != ESP_OK) {
            ESP_LOGE(TAG,"Raw frames not supported!!!");
            packets=0;
            return false;
        }       
    }
    packets-=cnt;
    return packets>0;
}

esp_err_t CTesterTask::finished(void) 
{
    Delay(1000);//to wait until sent packets gone
    leds_alarm_set(false);
    WiFi.set_mode(WiFi_MODE_NONE); 
    oled_clear(true);
    return ESP_OK;   
} 


