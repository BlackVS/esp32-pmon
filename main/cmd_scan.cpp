#include "app.h"
#include <map>

static const char *TAG = "SCAN";

std::map<uint64_t,wifi_target_t> _sniff_found_targets;

struct _scan_params_struct
{
    uint32_t channel; //=0 if all
    uint32_t duration;
    bool     f_aps;
    bool     f_stations;
    bool     f_verbose;

public:
    _scan_params_struct()
    {
        channel=0;
        duration=0; //20s byt default
        f_aps=0;
        f_stations=0;
        f_verbose=false;
    }
} _scan_params;





///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_scan_cmd(int argc, char **argv);

static struct {
    struct arg_lit *verbose;
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
    _scan_args.verbose = arg_lit0("v", "verbose", "Verbose to console");
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
                    authmode2str(ap.authmode)
                );
            }

        }
        printf("---------------------------------------------------------------------------------\n");
}


void dump(wifi_header_frame_control_t* ctrl){
    printf("------------ wifi_header_frame_control_t ---------------------\n");
    printf("protocol:%i type=%i subtype=%i to_ds=%i from_ds:%i ", ctrl->protocol, ctrl->type, ctrl->subtype, ctrl->to_ds, ctrl->from_ds);
    printf("more_frag:%i retry:%i pwr_mgmt:%i more_data:%i wep:%i strict:%i\n", ctrl->more_frag, ctrl->retry, ctrl->pwr_mgmt, ctrl->more_data, ctrl->wep, ctrl->strict);
    printf("--------------------------------------------------------------\n");
}

void dump(wifi_promiscuous_pkt_t* pr_pkt)
{
    wifi_pkt_rx_ctrl_t& rx=pr_pkt->rx_ctrl;
    printf("------------ wifi_pkt_rx_ctrl_t ---------------------\n");
    printf("rssi:%i rate:%i sig_mode:%i mcs:%i cwb:%i ", rx.rssi, rx.rate, rx.sig_mode, rx.mcs, rx.cwb);
    printf("smoothing:%i not_sounding:%i aggregation:%i ", rx.smoothing, rx.not_sounding, rx.aggregation);
    printf("stbc:%i fec_coding:%i sgi:%i noise_floor:%i ", rx.stbc, rx.fec_coding, rx.sgi, rx.noise_floor);
    printf("ampdu_cnt:%i channel:%i secondary_channel:%i ", rx.ampdu_cnt, rx.channel, rx.secondary_channel);
    printf("timestamp:%i ant:%i sig_len:%i rx_state:%i\n", rx.timestamp, rx.ant, rx.sig_len, rx.rx_state);
    printf("PAYLOAD: ");
    for(int i=0; i<16; i++){
        printf("%02x ",pr_pkt->payload[i]);
    }
    printf("\n");
}
///////////////////////////////////////////////////////////////////////////////////
//
// https://blog.podkalicki.com/esp32-wifi-sniffer/
extern "C" void scan_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) 
{
    if(_scan_params.f_verbose)
        printf(".");
    wifi_promiscuous_pkt_t* pr_pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pr_pkt->rx_ctrl;
    wifi_ieee80211_packet_t* packet = (wifi_ieee80211_packet_t*) &pr_pkt->payload;
    wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t*)&packet->hdr.frame_ctrl;

    uint8_t p_type    = frame_ctrl->type;
    uint8_t p_subtype = frame_ctrl->subtype;
    uint32_t sig_length = ctrl.sig_len;
    bool has_TA_RA = frame_ctrl->to_ds && frame_ctrl->from_ds;

    //uint8_t subtype = pr_pkt->payload[0];//buf[12]
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

    //mac_t macTo1(&pr_pkt->payload[16-12]);
    //mac_t macFrom1(&pr_pkt->payload[22-12]);


    mac_t mac_to   = packet->hdr.mac_to;
    mac_t mac_from = packet->hdr.mac_from;

    //printf("From: %s To: %s   <=>   ", mac2str(macFrom1).c_str(), (char*)mac2str(macTo1).c_str() );
    //printf("From: %s To: %s\n"       , mac2str(macFrom).c_str(),  (char*)mac2str(macTo).c_str()  );

    //snif radio's MACs
    
    bool is_bad = ctrl.channel==0 || ctrl.rssi>0;
    //if(has_TA_RA)
    {
        if(is_bad){
            printf("\nBAD: %s => %s l=%i t=%i s=%i\n", 
                    mac2str(mac_from).c_str(), 
                    mac2str(mac_to).c_str(), 
                    sig_length, p_type, p_subtype);
            dump(pr_pkt);
            dump(frame_ctrl);
        } 
        uint64_t macint = mac_to.to_int();
        
        if( !is_bad &&
            !mac_to.is_broadcast() &&
            !mac_to.is_multicast() &&
            mac_to.is_valid() &&
            _sniff_found_targets.count(macint)==0 ) 
        {
            wifi_target_t t(mac_to, TARGET_UNKNOWN, (uint8_t)ctrl.channel, NULL);
            _sniff_found_targets[macint]=t;
            //printf("\nFOUND : %s => [%s]  l=%i t=%i s=%i\n", mac2str(mac_from).c_str(), mac2str(mac_to).c_str(), sig_length, p_type, p_subtype);
            //dump(pr_pkt);
            //dump(frame_ctrl);
        }
        macint = mac_from.to_int();
        if( !is_bad &&
            !mac_from.is_broadcast() &&
            !mac_from.is_multicast() &&
            mac_from.is_valid() &&
            _sniff_found_targets.count(macint)==0 ) 
        {
            wifi_target_t t(mac_from, TARGET_UNKNOWN, (uint8_t)ctrl.channel, NULL);
            _sniff_found_targets[macint]=t;
            //printf("\nFOUND : [%s] => %s  l=%i t=%i s=%i\n", mac2str(mac_from).c_str(), mac2str(mac_to).c_str(), sig_length, p_type, p_subtype);
            //dump(pr_pkt);
            //dump(frame_ctrl);
        }
    }
}


typedef struct _snif_runtime_struct 
{
    bool              is_running;
    uint32_t          duration;
    uint32_t          duration1ch;
    bool              channel_auto;
    uint32_t          starttime;
    uint32_t          channel_starttime;
    TaskHandle_t      task_handle;
    SemaphoreHandle_t sem_task_over;

    _snif_runtime_struct()
    {
        is_running=false;
        channel_auto=true;
        duration1ch=0;
        duration   =0;
        starttime  =0;
        channel_starttime=0;
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
    size_t cnt=_sniff_found_targets.size();
    printf("\nTargets (stations/APs) found: %i\n", cnt);
    printf("------------------------------------------\n");
    printf("       MAC             Channel  Type  Info\n");
    printf("------------------------------------------\n");
    if(cnt>0) {
        for(auto& t:_sniff_found_targets){
            wifi_target_t& d=t.second;
            printf( "%s\t%2d\t%d\t%s\n", mac2str(d.mac).c_str(), d.channel, d.type, d.desc);
        }
    }
    printf("------------------------------------------\n");
    //WiFi.set_event_handler(NULL);
    ESP_LOGD(__FUNCTION__, "Stopped OK.");
    return ESP_OK;
}

extern "C" void scan_snif_task(void *pvParameters) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _snif_rt.starttime=_snif_rt.channel_starttime=millis();
    _snif_rt.is_running=true;

    if(_scan_params.f_verbose)
        printf("\nSniff channel %d: ",WiFi.get_channel());
    while (_snif_rt.is_running) 
    {
        uint32_t currentTime = millis();
        if ( currentTime >= _snif_rt.starttime + _snif_rt.duration ) 
            break;
        if ( _snif_rt.channel_auto && currentTime >= _snif_rt.channel_starttime + _snif_rt.duration1ch ) {
            WiFi.set_channel(WiFi.get_channel()+1, true);
            if(_scan_params.f_verbose)
                printf("\nSniff channel %d: ",WiFi.get_channel());
            _snif_rt.channel_starttime=millis();
        }
        vTaskDelay(10);
    }
    scan_sniffer_stop();
    ESP_LOGD(__FUNCTION__, "Left");
    vTaskDelete(NULL); //self delete
}

void scan_sniffer_start(void) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    if(_snif_rt.is_running){
        printf("Sniffer already running!");
        return;
    }

    _sniff_found_targets.clear();
    WiFi.set_mode(WiFi_MODE_NONE);

    uint32_t duration=_scan_params.duration;
    if(!duration)
        duration=5000;

    if(_scan_params.channel==0) 
    {   //auto channel switch
        _snif_rt.duration1ch=duration;
        _snif_rt.duration   =duration*WIFI_MAX_CH;
        _snif_rt.channel_auto=true;
         WiFi.set_channel(1);
    } else {
        //only one channel scan
        _snif_rt.duration1ch=duration;
        _snif_rt.duration   =duration;
        _snif_rt.channel_auto=false;
         WiFi.set_channel(_scan_params.channel);
    }
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
    //bool bChangeOnTheFly = _scan_params.is_running();

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
    _scan_params.channel=channel;

    // --time
    uint32_t time = 0;//use default value
    if (_scan_args.time->count) {
            time = _scan_args.time->ival[0]*1000;
    }
    _scan_params.duration=time;

    // target (aps, stations, all)
    bool fAPs       = _scan_args.all->count > 0 || _scan_args.aps->count > 0;
    bool fStations  = _scan_args.all->count > 0 || _scan_args.stations->count > 0;
    _scan_params.f_aps=fAPs;
    _scan_params.f_stations=fStations;
    _scan_params.f_verbose=_scan_args.verbose->count;

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
        scan_sniffer_start();
    }
    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

