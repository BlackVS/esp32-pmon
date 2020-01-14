#include "app.h"

static const char *TAG = __FILE__;

CRadarTask radar("Radar", 8192, 5, &pool_wifi_tasks);

CPacketsBuffer radar_packets(128);

///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_radar_cmd(int argc, char **argv);

static struct {
    struct arg_lit *start;
    struct arg_lit *stop;
    struct arg_int *channel;
    struct arg_lit *targets;
    struct arg_str *mac;
    struct arg_rex *type;
    struct arg_end *end;
} _radar_args;

void register_cmd_radar(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _radar_args.start   = arg_lit0(NULL, "start", "start packets monitor");
    _radar_args.stop    = arg_lit0(NULL, "stop", "stop packets monitor");
    _radar_args.channel = arg_int0("c", "channel", "<channel>", "WiFi channel to monitor. If not used or 0 - scan all channels.");
    _radar_args.targets = arg_lit0("t", "targets", "Monitor selected targets");
    _radar_args.mac     = arg_str0("m", "mac", "<AA:BB:CC:DD:EE:FF>", "Monitor target with specified mac address");
    _radar_args.type    = arg_rex0(NULL, NULL, "all|deauth", NULL, REG_ICASE, NULL);
    _radar_args.end = arg_end(1);
    const esp_console_cmd_t radar_cmd = {
        .command = "radar",
        .help = "Monitor RSSI of specified targets",
        .hint = NULL,
        .func = &do_radar_cmd,
        .argtable = &_radar_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&radar_cmd));
    ESP_LOGD(__FUNCTION__, "Started.");
}


///////////////////////////////////////////////////////////////////////////////////
//
// https://blog.podkalicki.com/esp32-wifi-sniffer/
static int32_t  pr_deauths=0;
static int32_t  pr_pckt_counter=0;
static int32_t  pr_rssi_sum=0;
static int32_t  pr_rssi_avg=-100;

extern "C" void radar_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) 
{
    wifi_promiscuous_pkt_t* pr_pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pr_pkt->rx_ctrl;
    wifi_ieee80211_packet_t* packet = (wifi_ieee80211_packet_t*) &pr_pkt->payload;
    wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t*)&packet->hdr.frame_ctrl;

    uint8_t p_type    = frame_ctrl->type;
    uint8_t p_subtype = frame_ctrl->subtype;
    uint32_t sig_length = ctrl.sig_len;


    // if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0 )) 
    //     monitor.pr_deauths++;

    // if (type == WIFI_PKT_MISC) 
    //     return;             // wrong packet type
    
    bool bOk=true;

    // if( type == WIFI_PKT_MGMT )
    // {
    //     switch(p_subtype){
    //         //drop deauth/disassociation frames
    //         case WiFi_MGMT_DISASSOCIATION:
    //             //ESP_LOGD(TAG, " WiFi_MGMT_DISASSOCIATION ");
    //             return;
    //         case WiFi_MGMT_DEAUTHENTICATION:
    //             //ESP_LOGD(TAG, " WiFi_MGMT_DEAUTHENTICATION ");
    //             return;
    //         // drop beacon frames, probe requests/responses and 
    //         case WiFi_MGMT_BEACON:
    //             //ESP_LOGD(TAG, " WiFi_MGMT_BEACON ");
    //             return;
    //         case WiFi_MGMT_PROBE_REQ:
    //             //ESP_LOGD(TAG, " WiFi_MGMT_PROBE_REQ ");
    //             return;
    //         case WiFi_MGMT_PROBE_RES:
    //             //ESP_LOGD(TAG, " WiFi_MGMT_PROBE_RES ");
    //             return;
    //     }
    // }
    // mac_t mac_to   = packet->hdr.mac_to;
    // mac_t mac_from = packet->hdr.mac_from;
    //snif radio's MACs
    // bool is_bad = ctrl.channel==0 || ctrl.rssi>0;
    if(bOk) {
        pr_pckt_counter++;
        pr_rssi_sum += ctrl.rssi;
        if(pr_pckt_counter){
            pr_rssi_avg=pr_rssi_sum/pr_pckt_counter;
        }
    }
}

void radar_oled_draw()
{
    //if (!engine.nextFrame()) 
    //    return;
    oled_clear();

    oled_print(0,0,"CH:");
    oled_print(24, 0, WiFi.get_channel(), STYLE_BOLD);
    //oled_printf(0,0,STYLE_BOLD, "CH: %i", WiFi.get_channel());

    oled_print(0,   56, "RSSI", STYLE_BOLD);

    static int32_t last_lv=-100;
    if(pr_rssi_avg!=-100) {
        oled_print(104, 56, pr_rssi_avg, STYLE_BOLD);
        last_lv=pr_rssi_avg;
    } else
        oled_print(104, 56, last_lv, STYLE_BOLD);

    oled_print(80,  0, "CNT:");
    oled_print(112, 0, pr_pckt_counter, STYLE_BOLD);

    int32_t yt = 11;
    int32_t yb = 51;
    int32_t yh = yb-yt;

    oled_drawHLine(0, yt-2, 127);
    oled_drawHLine(0, yb  , 127);

    int32_t vmax=radar_packets.get_max_pckt_rssi();
    vmax=MAX(-50,vmax);
    //oled_print(58, 0, vmax, STYLE_BOLD);
    vmax+=100;

    int32_t vavg=radar_packets.get_avg_pckt_rssi();
    //oled_print(58, 56, vavg, STYLE_BOLD);
    vavg+=100;

    if(vmax!=0){
        for(int i=0; i<radar_packets.len(); i++ ){
            int32_t v = radar_packets.get(i).rssi_avg;
            if(v==0) continue;
            v+=100;
            int32_t dy = (v*yh)/vmax;
            oled_drawVLine(i, yb-dy, yb);
            //printf("%i %i %i %i %i\n", i, v, dy, yb-dy, yb);
        }
    }
    oled_refresh();
}

esp_err_t CRadarTask::starting(void) 
{
    pr_deauths=0; 
    pr_pckt_counter=0;
    pr_rssi_sum=0;
    pr_rssi_avg=-100;
    lastDrawTime=0;
    radar_packets.clear();
    starttime=channel_starttime=millis();
    return ESP_OK;
}

bool CRadarTask::execute(void) 
{
    uint32_t currentTime = millis();

    if ( channel_auto && currentTime >= channel_starttime + channel_dur ) 
    {
        WiFi.set_channel(WiFi.get_channel()+1, true);
        channel_starttime=millis();
        //oled_printf_refresh(0,24,STYLE_NORMAL,"Channel: %i", WiFi.get_channel());
        pr_pckt_counter = 0;
        pr_deauths  = 0;
        pr_rssi_sum = 0;
        pr_rssi_avg = -100;
    }

    if ( currentTime - lastDrawTime >= oled_refresh_dur ) 
    {
        lastDrawTime = currentTime;
        PACKET_STAT packet;
        packet.pckt_counter=pr_pckt_counter;
        packet.rssi_avg    =pr_rssi_avg;
        packet.pr_deauths  =0;//not use here
        radar_packets.add(packet);
    
        //////////////////////////////
        //OLED always
        radar_oled_draw();

        //ALARM
        //leds_alarm_set(pr_deauths>=_death_alarm_thresh);

        lastDrawTime = currentTime;
    }

    return true;
}

esp_err_t CRadarTask::finished(void) 
{
    WiFi.set_event_handler(NULL); //to avoid recursive calls
    WiFi.set_mode(WiFi_MODE_NONE);
    oled_clear(true);
    return ESP_OK;
} 

esp_err_t CRadarTask::init(void) 
{ 
    WiFi.set_mode(WiFi_MODE_NONE);

    if(channel==0) 
    {   //auto channel switch
        channel_auto  = true;
        WiFi.set_channel(1);
    } else {
        //only one channel scan
        channel_auto  = false;
        WiFi.set_channel(channel);
    }
    oled_printf_refresh(0,24,STYLE_NORMAL,"Channel: %i", WiFi.get_channel());
    WiFi.set_promiscuous_callback(radar_sniffer_callback);
    WiFi.set_mode(WiFi_MODE_PROMISCUOUS);
    //WiFi.set_event_handler(scan_events_callback); - maintained via pool
    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_radar_cmd(int argc, char **argv)
{
    ESP_LOGD(__FUNCTION__, "Enter...");
    int nerrors = arg_parse(argc, argv, (void **)&_radar_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, _radar_args.end, argv[0]);
        return 0;
    }

    bool bChangeOnTheFly = radar.is_running() && !_radar_args.stop->count;
    // --stop 
    if (_radar_args.stop->count) {
        radar.stop(true);
        return 0;
    }

    // --channel
    if (_radar_args.channel->count) 
    {
        int channel=_radar_args.channel->ival[0];
        WiFi.set_channel(channel, bChangeOnTheFly);
        radar.channel=channel;
    }

    // --start
    if (_radar_args.start->count) {
        radar.start();
    }

    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

