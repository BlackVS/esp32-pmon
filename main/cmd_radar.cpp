#include "app.h"

//static const char *TAG = __FILE__;

CRadarTask radar("Radar", 8192, 5, &pool_wifi_tasks);


#define RSSI_MIN -90
#define RSSI_MAX -20
#define RSSI_K_FADE 0.9f

CPacketsBuffer radar_timeseries(128);
PACKET_STAT    radar_hist[WIFI_MAX_CH];

///////////////////////////////////////////////////////////////////////////////////
//
//
int do_radar_cmd(int argc, char **argv);

static struct {
    struct arg_lit *start;
    struct arg_lit *stop;
    struct arg_int *channel;
    struct arg_lit *targets;
    struct arg_str *mac;
    struct arg_rex *ptype;
    struct arg_rex *oledmode;
    struct arg_end *end;
} _radar_args;

void register_cmd_radar(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _radar_args.start   = arg_lit0(NULL, "start", "start packets monitor");
    _radar_args.stop    = arg_lit0(NULL, "stop", "stop packets monitor");
    _radar_args.channel = arg_int0("c", "channel", "<channel>", "WiFi channel to monitor. If not used or 0 - scan all channels.");
    _radar_args.targets = arg_lit0("t", "targets", "Monitor selected targets");
    _radar_args.mac     = arg_str0("m", "mac", "all|AA:BB:CC:DD:EE:FF", "Monitor target with specified mac address");
    _radar_args.ptype   = arg_rex0("p", "packets", "all|deauth",     "packets to monitor", REG_ICASE, NULL);
    _radar_args.oledmode= arg_rex0("o", "oled"   , "none|time|hist", "OLED mode", REG_ICASE, NULL);
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

    //uint8_t p_type    = frame_ctrl->type;
    uint8_t p_subtype = frame_ctrl->subtype;

    mac_t mac_from = packet->hdr.mac_from;

    // uint32_t sig_length = ctrl.sig_len;


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
    //snif radio's MACs

    if(ctrl.channel==0 || ctrl.rssi>0)
        return;//

    if(radar.mac.is_valid())
    {
        bOk = bOk && mac_from==radar.mac;
    }
    if(radar.ptype==RADAR_PACKETS_DEAUTH) 
        bOk = bOk && type==WIFI_PKT_MGMT && (p_subtype == WiFi_MGMT_DISASSOCIATION || p_subtype==WiFi_MGMT_DEAUTHENTICATION);

    if(bOk) {
        pr_pckt_counter++;
        pr_rssi_sum += ctrl.rssi;
        pr_rssi_avg=-100;
        if(pr_pckt_counter){
            pr_rssi_avg=pr_rssi_sum/pr_pckt_counter;
        }
        if(ctrl.channel>=1&&ctrl.channel<=WIFI_MAX_CH)
        {
            int v=-RSSI_MIN+MIN(RSSI_MAX, MAX(RSSI_MIN, pr_rssi_avg));
            radar_hist[ctrl.channel-1].rssi_avg=v;
        }
    }
}

void radar_oled_rssi()
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

    int32_t vmax=radar_timeseries.get_max_pckt_rssi();
    vmax=MAX(-50,vmax);
    //oled_print(58, 0, vmax, STYLE_BOLD);
    vmax+=100;

    int32_t vavg=radar_timeseries.get_avg_pckt_rssi();
    //oled_print(58, 56, vavg, STYLE_BOLD);
    vavg+=100;

    if(vmax!=0){
        for(int i=0; i<radar_timeseries.len(); i++ ){
            int32_t v = radar_timeseries.get(i).rssi_avg;
            if(v==0) continue;
            v+=100;
            int32_t dy = (v*yh)/vmax;
            oled_drawVLine(i, yb-dy, yb);
            //printf("%i %i %i %i %i\n", i, v, dy, yb-dy, yb);
        }
    }
    oled_refresh();
}

void radar_oled_channels()
{
    //if (!engine.nextFrame()) 
    //    return;
    oled_clear();

    int32_t yt = 10;
    int32_t yb = 52;
    int32_t yh = yb-yt+1;
    oled_drawHLine(0, yb  , 127);

    int xbar=8;
    int xstep=9;
    for(int c=1; c<=WIFI_MAX_CH; c++)
    {
        int v=radar_hist[c-1].rssi_avg;
        v=(yh*v)/(RSSI_MAX-RSSI_MIN);
        int x1=1+(c-1)*xstep;
        int x2=x1+xbar-1;
        int y1=yb;
        int y2=yb-v;
        oled_printf(x1, 56, STYLE_NORMAL, "%c", (c<10) ? ('0'+c) : ('A'+c-10) );
        if(v) 
            oled_fillRect(x1, y1, x2, y2);
    }

    oled_refresh();
}

esp_err_t CRadarTask::starting(void) 
{
    reset_stats();
    lastDrawTime=0;
    radar_timeseries.clear();
    starttime=channel_starttime=millis();
    return ESP_OK;
}

void CRadarTask::reset_stats(void)
{
    pr_deauths=0; 
    pr_pckt_counter=0;
    pr_rssi_sum=0;
    pr_rssi_avg=-100;
    stats_starttime=millis();
}

bool CRadarTask::execute(void) 
{
    uint32_t currentTime = millis();

    int32_t v=pr_rssi_avg;
    v=-RSSI_MIN+MIN(RSSI_MAX, MAX(RSSI_MIN, v));
    v=(10*v)/(RSSI_MAX-RSSI_MIN);
    v=MIN(10, MAX(0, v));
    if(v>0)
        leds_alarm_set(true, radar.ptype==RADAR_PACKETS_ALL?LED_WHITE:LED_RED, 0.9f, v, false );

    if ( channel_auto && currentTime >= channel_starttime + channel_dur ) 
    {
        WiFi.set_channel(WiFi.get_channel()+1, true);
        channel_starttime=millis();
        reset_stats();
    }


    if ( radar.oledmode!=RADAR_OLED_MODE_NONE ) 
    {
        if( currentTime - lastDrawTime >= oled_refresh_dur )
        {
            lastDrawTime = currentTime;
            PACKET_STAT packet;
            packet.pckt_counter=pr_pckt_counter;
            packet.rssi_avg    =pr_rssi_avg;
            packet.pr_deauths  =0;//not use here
            radar_timeseries.add(packet);
        
            //////////////////////////////
            switch(radar.oledmode){
                case RADAR_OLED_MODE_TIME:
                    radar_oled_rssi();
                    break;
                case RADAR_OLED_MODE_HIST:
                    radar_oled_channels();
                    break;
                default:
                    break;
            }
//            if(WiFi.get_channel()==1) 
            for(int c=0; c<WIFI_MAX_CH; c++)
                radar_hist[c].rssi_avg=int(radar_hist[c].rssi_avg*RSSI_K_FADE);

            if(!channel_auto)
                reset_stats();

            lastDrawTime = currentTime;
        }
    } else {
        if(!radar.oledcleared){
            oled_clear();
            oled_refresh();
            radar.oledcleared=true;
        }
    }

    return true;
}

esp_err_t CRadarTask::finished(void) 
{
    WiFi.set_event_handler(NULL); //to avoid recursive calls
    WiFi.set_mode(WiFi_MODE_NONE);
    leds_alarm_set(false);
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
    memset(radar_hist,0,sizeof(radar_hist));
    WiFi.set_promiscuous_callback(radar_sniffer_callback);
    WiFi.set_mode(WiFi_MODE_PROMISCUOUS);
    //WiFi.set_event_handler(scan_events_callback); - maintained via pool
    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////////
//
//
int do_radar_cmd(int argc, char **argv)
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

    // --packets
    if(_radar_args.ptype->count)
    {
        const char* ptype=_radar_args.ptype->sval[0];
        if(strcmp(ptype,"all")==0){
            radar.ptype=RADAR_PACKETS_ALL;
        } else
        if(strcmp(ptype,"deauth")==0){
            radar.ptype=RADAR_PACKETS_DEAUTH;
        } else
            {
            printf("Wrong parameter: %s\n", ptype);
            return 0;
        }
}

    // --oledmode
    if(_radar_args.oledmode->count)
    {
        const char* oledmode=_radar_args.oledmode->sval[0];
        if(strcmp(oledmode,"none")==0){
            radar.set_oledMode(RADAR_OLED_MODE_NONE);
        } else
        if(strcmp(oledmode,"hist")==0){
            radar.set_oledMode(RADAR_OLED_MODE_HIST);
        } else
        if(strcmp(oledmode,"time")==0){
            radar.set_oledMode(RADAR_OLED_MODE_TIME);
        } else
        {
            printf("Wrong parameter: %s\n", oledmode);
            return 0;
        }
    }

    // --channel
    if (_radar_args.channel->count) 
    {
        int channel=_radar_args.channel->ival[0];
        if(channel==0){
            radar.channel_auto=true;
        } else {
            WiFi.set_channel(channel, bChangeOnTheFly);
            radar.channel_auto=false;
        }
        radar.channel=channel;
    }

    // --mac
    if (_radar_args.mac->count) 
    {
        const char* macstr =_radar_args.mac->sval[0];
        if(strcmp(macstr,"all")==0)
        {
            radar.mac.set(0,0,0,0,0,0);
        } else
        {
            bool r=radar.mac.set(macstr);
            if(!r) {
                printf("Wrong parameter: %s\n", macstr);
                return 0;
            }
        }
    }            

    // --start
    if (_radar_args.start->count) {
        radar.start();
    }

    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

void CRadarTask::set_oledMode(RADAR_OLED_MODE m)
{
    switch(m){
        case RADAR_OLED_MODE_TIME:
            channel_dur = 500;
            oled_refresh_dur = 250;
            break;
        case RADAR_OLED_MODE_HIST:
            channel_dur = 100;
            oled_refresh_dur = 500;
            break;
        default:
            channel_dur = 100;
            oled_refresh_dur = 500;
            oledcleared=false;
            break;
    }
    oledmode=m;

}
