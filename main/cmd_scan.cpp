#include "app.h"

static const char *TAG = __FILE__;


class CScanRuntime
{
    bool     _is_running;
    //// args
    uint32_t channel; //=0 if all
    uint32_t duration;
    bool     f_aps;
    bool     f_stations;
    //// task 
    TaskHandle_t task;
    SemaphoreHandle_t sem_task_over;

public:
    CScanRuntime()
    {
        clear();
    }

    void clear(void)
    {
        _is_running=false;
        channel=0;
        duration=20; //20s byt default
        f_aps=0;
        f_stations=0;
        task=NULL;
        sem_task_over=NULL;

    }

    bool is_running(void){
        return _is_running;
    }

    void set_channel(uint32_t ch){
        channel =  MIN(WIFI_MAX_CH, MAX(0,ch));
    }

    void set_duration(uint32_t dur){
        duration = dur;
    }

    void set_target(bool fAPS, bool fStations){
        f_aps=fAPS;
        f_stations=fStations;
    }

} _scan_rt;





///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_scan_cmd(int argc, char **argv);

static struct {
    struct arg_lit *all;
    struct arg_lit *aps;
    struct arg_lit *stations;
    struct arg_lit *results;
    struct arg_int *channel;
    struct arg_int *time;
    struct arg_end *end;
} _scan_args;

void register_cmd_scan(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _scan_args.all = arg_lit0("a", "all", "Scans for APs+stations");
    _scan_args.aps = arg_lit0("p", "aps", "Scans for APs only");
    _scan_args.stations = arg_lit0("s", "stations", "Scans for Stations only");
    _scan_args.channel = arg_int0("c", "channel", "<channel>", "WiFi channel to scan only. If not used or 0 - scan all channels.");
    _scan_args.time = arg_int0("t", "time", "<seconds>", "How long in seconds it should scan.");
    _scan_args.results = arg_lit0("r", "results", "Show last scan results");

    _scan_args.end = arg_end(1);
    const esp_console_cmd_t scan_cmd = {
        .command = "scan",
        .help = "Scan APs/Stations",
        .hint = NULL,
        .func = &do_scan_cmd,
        .argtable = &_scan_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&scan_cmd));
    ESP_LOGD(__FUNCTION__, "Started.");
}


static void cmd_results(void)
{
        uint32_t cnt = WiFi.scan_APs_get_count();
        if(cnt==0){
            printf("No APs found or scan not run yet!\n");
            return;
        }
        //printf("%i APs found:\n", cnt);
        printf("---------------------------------------------------------------------------------\n");
        printf(" #          MAC          CH   RSSI    SSID              MODE\n");
        printf("---------------------------------------------------------------------------------\n");
        for(int i=0; i<cnt; i++)
        {
            wifi_ap_record_t ap;
            if( WiFi.scan_APs_get_data(i, ap) )
            {
                printf("%2i   %02x:%02x:%02x:%02x:%02x:%02x   %2d   %3d     %-16s  %s\n", 
                    i,
                    ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5],
                    ap.primary,
                    ap.rssi,
                    ap.ssid,
                    WiFi.authmode2str(ap.authmode)
                );
            }

        }
        printf("---------------------------------------------------------------------------------\n");
}


///////////////////////////////////////////////////////////////////////////////////
//
// https://blog.podkalicki.com/esp32-wifi-sniffer/
extern "C" void scan_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) 
{
    //printf(".");
    wifi_promiscuous_pkt_t* pr_pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pr_pkt->rx_ctrl;
    wifi_ieee80211_packet_t* packet = (wifi_ieee80211_packet_t*) &pr_pkt->payload;
    wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t*)&packet->hdr.frame_ctrl;

    uint8_t p_type    = frame_ctrl->type;
    uint8_t p_subtype = frame_ctrl->subtype;
    

    uint8_t subtype = pr_pkt->payload[0];//buf[12]
    //ESP_LOGD(TAG,"%x %x %x %x\n",p_type,p_subtype,type,subtype);
    
    //if (type == WIFI_PKT_MGMT && (subtype == 0xA0 || subtype == 0xC0 )) 
    if( type == WIFI_PKT_MGMT )
    {
        switch(p_subtype){
            //drop deauth/disassociation frames
            case WiFi_MGMT_DISASSOCIATION:
                //ESP_LOGD(TAG, " WiFi_MGMT_DISASSOCIATION ");
                return;
            case WiFi_MGMT_DEAUTHENTICATION:
                //ESP_LOGD(TAG, " WiFi_MGMT_DEAUTHENTICATION ");
                return;
            // drop beacon frames, probe requests/responses and 
            case WiFi_MGMT_BEACON:
                //ESP_LOGD(TAG, " WiFi_MGMT_BEACON ");
                return;
            case WiFi_MGMT_PROBE_REQ:
                //ESP_LOGD(TAG, " WiFi_MGMT_PROBE_REQ ");
                return;
            case WiFi_MGMT_PROBE_RES:
                //ESP_LOGD(TAG, " WiFi_MGMT_PROBE_RES ");
                return;
        }
    }
    // only allow data frames
    // if(buf[12] != 0x08 && buf[12] != 0x88) return;

    uint8_t* macTo1   = &pr_pkt->payload[16-12];
    uint8_t* macFrom1 = &pr_pkt->payload[22-12];


    uint8_t* macTo   = packet->hdr.mac_to;
    uint8_t* macFrom = packet->hdr.mac_from;

    printf("From: %02x:%02x:%02x:%02x:%02x:%02x To: %02x:%02x:%02x:%02x:%02x:%02x   <=>   ", 
            macFrom1[0],macFrom1[1],macFrom1[2],macFrom1[3],macFrom1[4],macFrom1[5],
            macTo1[0],macTo1[1],macTo1[2],macTo1[3],macTo1[4],macTo1[5]
            );

    printf("From: %02x:%02x:%02x:%02x:%02x:%02x To: %02x:%02x:%02x:%02x:%02x:%02x\n", 
            macFrom[0],macFrom[1],macFrom[2],macFrom[3],macFrom[4],macFrom[5],
            macTo[0],macTo[1],macTo[2],macTo[3],macTo[4],macTo[5]
            );

    //if (macBroadcast(macTo) || macBroadcast(macFrom) || !macValid(macTo) || !macValid(macFrom) || macMulticast(macTo) ||
    //    macMulticast(macFrom)) return;


  //printf(".");
}


typedef struct _snif_runtime_struct 
{
    bool              is_running;
    uint32_t          duration;
    uint32_t          starttime;
    TaskHandle_t      task_handle;
    SemaphoreHandle_t sem_task_over;

    _snif_runtime_struct()
    {
        is_running=false;
        duration=20000;
        starttime=0;
    }
} snif_runtime_t;

static snif_runtime_t _snif_rt;



///////////////////////////////////////////////////////////////////////////////////
//
//
static esp_err_t scan_sniffer_stop(void)
{
    ESP_LOGD(__FUNCTION__, "Stopping task");
    WiFi.set_mode(WiFi_MODE_NONE);
    _snif_rt.is_running = false; //trigger to stop
    //WiFi.set_event_handler(NULL);
    ESP_LOGD(__FUNCTION__, "Stopped OK.");
    return ESP_OK;
}

extern "C" void scan_snif_task(void *pvParameters) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _snif_rt.starttime=millis();
    _snif_rt.is_running=true;

    while (_snif_rt.is_running) 
    {
        uint32_t currentTime = millis();
        if ( currentTime >= _snif_rt.starttime + _snif_rt.duration ) 
            break;
        vTaskDelay(10);
    }
    scan_sniffer_stop();
    ESP_LOGD(__FUNCTION__, "Left");
    vTaskDelete(NULL); //self delete
}

void scan_sniffer_start(uint32_t duration=0) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    if(_snif_rt.is_running){
        printf("Sniffer already running!");
        return;
    }
    if(duration)
    {
        _snif_rt.duration=duration;
    }
    WiFi.set_mode(WiFi_MODE_NONE);
    WiFi.set_promiscuous_callback(scan_sniffer_callback);
    WiFi.set_mode(WiFi_MODE_PROMISCUOUS);
    BaseType_t ret = xTaskCreate( scan_snif_task, 
                                 "Scan: sniffer", 
                                CONFIG_PMON_TASK_STACK_SIZE,
                                NULL, 
                                CONFIG_PMON_TASK_PRIORITY, 
                                NULL);
    if(ret != pdTRUE)
    {
        ESP_LOGD(__FUNCTION__, "FAILED to start!!!");
        scan_sniffer_stop();
        return ;
    }
    ESP_LOGD(__FUNCTION__, "Started OK.");
}

///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_scan_cmd(int argc, char **argv)
{
    ESP_LOGD(__FUNCTION__, "Enter...");
    int nerrors = arg_parse(argc, argv, (void **)&_scan_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, _scan_args.end, argv[0]);
        return 0;
    }
    //bool bChangeOnTheFly = _scan_rt.is_running();

    // target (aps, stations, all)
    if (_scan_args.results->count > 0){
        //show current or last scan results, rest parameters just ignored
        printf("Last scan results are: \n");
        cmd_results();
        return 0;
    }

    // --channel
    uint32_t channel = 0;//all
    if (_scan_args.channel->count) {
        channel = _scan_args.channel->ival[0];
    }
    _scan_rt.set_channel(channel);

    // --time
    uint32_t time = 0;//use default value
    if (_scan_args.time->count) {
        time = _scan_args.time->ival[0];
    }
    _scan_rt.set_duration(time);

    // target (aps, stations, all)
    bool fAPs       = _scan_args.all->count > 0 || _scan_args.aps->count > 0;
    bool fStations  = _scan_args.all->count > 0 || _scan_args.stations->count > 0;
    _scan_rt.set_target(fAPs, fStations);

    ///////////////// start scan itself
    if(fAPs){
        WiFi.scan_APs();
        uint32_t cnt = WiFi.scan_APs_get_count();
        if(cnt==0){
            printf("No APs found!\n");
        } else
            cmd_results();
    }
    if(fStations)
    {
        scan_sniffer_start(time);
    }
    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

