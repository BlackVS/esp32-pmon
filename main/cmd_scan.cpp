#include "app.h"

static const char *TAG = __FILE__;

CSnifferTask sniffer("Sniffer", 8192, 5, &pool_wifi_tasks);

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

        targets_print();
}


void aps2targets(void)
{
    uint32_t cnt = WiFi.scan_APs_get_count();
    if(cnt==0)
        return;
    for(int i=0; i<cnt; i++)
    {
        wifi_ap_record_t ap;
        if( WiFi.scan_APs_get_data(i, ap) )
        {
            mac_t mac(ap.bssid);
            mac_t mac_link;
            target_add(mac, TARGET_AP, ap.primary, (char*)ap.ssid, mac_link);
        }

    }
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
    if(sniffer.f_verbose)
        printf(".");
    wifi_promiscuous_pkt_t* pr_pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pr_pkt->rx_ctrl;
    wifi_ieee80211_packet_t* packet = (wifi_ieee80211_packet_t*) &pr_pkt->payload;
    wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t*)&packet->hdr.frame_ctrl;

    uint8_t p_type    = frame_ctrl->type;
    uint8_t p_subtype = frame_ctrl->subtype;
    uint32_t sig_length = ctrl.sig_len;
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
    mac_t mac_to   = packet->hdr.mac_to;
    mac_t mac_from = packet->hdr.mac_from;

    //snif radio's MACs
    bool is_bad = ctrl.rx_state!=0 || ctrl.channel==0 || ctrl.rssi>0;
    {
        if(is_bad&&sniffer.f_verbose){
            printf("\nBAD: %s => %s l=%i t=%i s=%i\n", 
                    mac2str(mac_from).c_str(), 
                    mac2str(mac_to).c_str(), 
                    sig_length, p_type, p_subtype);
            dump(pr_pkt);
            dump(frame_ctrl);
        } 
        
        if( !is_bad &&
            !mac_to.is_broadcast() &&
            !mac_to.is_multicast() &&
            mac_to.is_valid() &&
            !target_exists(mac_to) ) 
        {
            wifi_target_t t;
            mac_t mac_link;
            char * desc=NULL;
            if(target_get(mac_from,t)) {
                mac_link=t.mac;
                desc=t.desc;
            }
            target_add(mac_to, TARGET_STATION, (uint8_t)ctrl.channel, desc, mac_link);
        }
        if( !is_bad &&
            !mac_from.is_broadcast() &&
            !mac_from.is_multicast() &&
            mac_from.is_valid() &&
            !target_exists(mac_from) ) 
        {
            wifi_target_t t;
            mac_t mac_link;
            char * desc=NULL;
            if(target_get(mac_to,t)) {
                mac_link=t.mac;
                desc=t.desc;
            }
            target_add(mac_from, TARGET_STATION, (uint8_t)ctrl.channel, desc, mac_link);
        }
    }
}


/*void scan_events_callback(WiFi_EVENT ev, uint32_t arg)
{
    ESP_LOGD(__FUNCTION__, "Event: %i", ev);
    if(ev==WiFi_EVENT_MODE_CHANGED){
        ESP_LOGD(__FUNCTION__, "Event: WiFi_EVENT_MODE_CHANGED");
        WiFi_MODES mode = (WiFi_MODES)arg;
        if(sniffer.is_running() && mode!=WiFi_MODE_PROMISCUOUS) //i.e changing mode to not sniffer ones
        {
            ESP_LOGD(__FUNCTION__, "Event: gracefully shutdown");
            //gracefully shutdown if mode changed externally
            sniffer.stop(true);
        }
    }
}*/

esp_err_t CSnifferTask::starting(void) 
{
    leds_alarm_set(true, LED_YELLOW, 0.7f);
    starttime=channel_starttime=millis();
    if(f_verbose)
        printf("\nSniff channel %d: ",WiFi.get_channel());
    return ESP_OK;
}

bool CSnifferTask::execute(void) 
{
    uint32_t currentTime = millis();
    if ( currentTime >= starttime + durationTotal ) 
        return false;
    if ( channel_auto && currentTime >= channel_starttime + duration1ch ) 
    {
        WiFi.set_channel(WiFi.get_channel()+1, true);
        if(f_verbose)
            printf("\nSniff channel %d: ",WiFi.get_channel());
        channel_starttime=millis();
        oled_printf_refresh(0,24,STYLE_NORMAL,"Channel: %i", WiFi.get_channel());
    }
    return true;
}

esp_err_t CSnifferTask::finished(void) 
{
    targets_print();
    oled_printf_refresh(0,32,STYLE_NORMAL,"Stations: %i", targets.size());
    oled_print(0,40,"Sniffer stopped.",STYLE_NORMAL, true);
    leds_alarm_set(false);
    WiFi.set_mode(WiFi_MODE_NONE); 
    return ESP_OK;
} 

esp_err_t CSnifferTask::init(void) 
{ 
    WiFi.set_mode(WiFi_MODE_NONE);

    if(!duration)
        duration=5000;

    if(channel==0) 
    {   //auto channel switch
        duration1ch   = duration;
        durationTotal = duration*WIFI_MAX_CH;
        channel_auto  = true;
        WiFi.set_channel(1);
    } else {
        //only one channel scan
        duration1ch   = duration;
        durationTotal = duration;
        channel_auto  = false;
        WiFi.set_channel(channel);
    }
    oled_printf_refresh(0,24,STYLE_NORMAL,"Channel: %i", WiFi.get_channel());
    WiFi.set_promiscuous_callback(scan_sniffer_callback);
    WiFi.set_mode(WiFi_MODE_PROMISCUOUS);
    //WiFi.set_event_handler(scan_events_callback); - maintained via pool

    return ESP_OK;
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
    sniffer.channel=channel;

    // --time
    uint32_t time = 0;//use default value
    if (_scan_args.time->count) {
            time = _scan_args.time->ival[0]*1000;
    }
    sniffer.duration=time;

    // target (aps, stations, all)
    bool fAPs       = _scan_args.all->count > 0 || _scan_args.aps->count > 0;
    bool fStations  = _scan_args.all->count > 0 || _scan_args.stations->count > 0;
    sniffer.f_verbose=_scan_args.verbose->count;

    if(fAPs||fStations)
    {   //stop all other WiFi tasks  
        pool_wifi_tasks.tasks_stop_all();
        //and via events handlers
        WiFi.set_mode(WiFi_MODE_NONE);
    }

    ///////////////// start scan itself
    if(fAPs){
        oled_print(0,0,"Scanning APs...",STYLE_NORMAL, true);
        WiFi.scan_APs();
        uint32_t cnt = WiFi.scan_APs_get_count();
        oled_printf(0,8,STYLE_NORMAL, "APs found : %i", cnt);
        if(cnt==0){
            printf("No APs found!\n");
        } else 
            cmd_results();
        aps2targets();
    }
    if(fStations)
    {
        oled_print(0,16,"Sniffer starts...");
        oled_refresh();
        esp_err_t err=sniffer.start();
        if(err!=ESP_OK)
        {
            ESP_LOGE(TAG, "Can't start sniffer!!!");
        }
    }
    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

