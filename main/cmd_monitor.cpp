#include "app.h"


__attribute__ ((unused)) 
static const char *TAG = __FILE__;

static int _death_alarm_thresh=CONFIG_PMON_DEAUTH_DETECT_LEVEL;

CMonitorTask monitor("Monitor", 8192, 5, &pool_wifi_tasks);

static struct {
    struct arg_lit *verbose;
    struct arg_int *channel;
    struct arg_lit *start;
    struct arg_lit *stop;
    struct arg_end *end;
} _pmon_args;

CPacketsBuffer pmon_packets(PMON_DEFAULT_BUFFER_LEN);

///////////////////////////////////////////////////////////////////////////////////
//
//
extern "C" void pmon_promiscuous_callback(void* buf, wifi_promiscuous_pkt_type_t type) 
{
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

  if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0 )) 
    monitor.pr_deauths++;

  if (type == WIFI_PKT_MISC) 
    return;             // wrong packet type
  
  monitor.pr_pckt_counter++;
  monitor.pr_rssi_sum += ctrl.rssi;
  if(monitor.pr_pckt_counter){
      monitor.pr_rssi_avg=monitor.pr_rssi_sum/monitor.pr_pckt_counter;
  }

  //printf(".");
}





void monitor_oled_draw()
{
    //if (!engine.nextFrame()) 
    //    return;
    oled_clear();

    oled_print(0,0,"CH:");
    oled_print(24, 0, WiFi.get_channel(), STYLE_BOLD);
    //oled_printf(0,0,STYLE_BOLD, "CH: %i", WiFi.get_channel());

    oled_print(0,  56, "LV:");
    oled_print(16, 56, monitor.pr_rssi_avg, STYLE_BOLD);

    oled_print(96,  0, "P:");
    oled_print(112, 0, monitor.pr_pckt_counter, STYLE_BOLD);

    oled_print(96,  56, "D:");
    oled_print(112, 56, monitor.pr_deauths, STYLE_BOLD);


    uint32_t yt = 11;
    uint32_t yb = 51;
    uint32_t yh = yb-yt+1;

    oled_drawHLine(0, yt-2, 127);
    oled_drawHLine(0, yb  , 127);

    uint32_t vmax=pmon_packets.get_max_pckt_counter();
    oled_print(58, 0, vmax, STYLE_BOLD);

    uint32_t vavg=pmon_packets.get_avg_pckt_counter();
    oled_print(58, 56, vavg, STYLE_BOLD);

    if(vmax>0){
        for(int i=0; i<pmon_packets.len(); i++ ){
            uint32_t v  = pmon_packets.get(i).pckt_counter;
            uint32_t dy = v;
            if(vmax>yh)
                dy=(v*yh)/vmax;
            oled_drawVLine(i, yb-dy, yb);
            //printf("%i %i %i %i %i\n", i, v, dy, yb-dy, yb);
        }
    }
    oled_refresh();
}

// click - change channel
void pmon_tpad_onClick(TOUCHPAD_EVENT, int value)
{
    WiFi.set_channel( WiFi.get_channel()+1, true );
    pmon_packets.clear();
}


///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_pmon_cmd(int argc, char **argv)
{
    ESP_LOGD(__FUNCTION__, "Enter...");
    int nerrors = arg_parse(argc, argv, (void **)&_pmon_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, _pmon_args.end, argv[0]);
        return 0;
    }

    bool bChangeOnTheFly = monitor.is_running() && !_pmon_args.stop->count;
    // --stop 
    if (_pmon_args.stop->count) {
        monitor.stop(true);
        return 0;
    }

    // --channel
    if (_pmon_args.channel->count) 
    {
        WiFi.set_channel( _pmon_args.channel->ival[0] , bChangeOnTheFly);
        if(bChangeOnTheFly)
          pmon_packets.clear();
    }

    // --verbose
    monitor.f_verbose = _pmon_args.verbose->count > 0;

    // --start
    if (_pmon_args.start->count) {
        monitor.start();
    }
    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
//
//
void register_cmd_monitor(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _pmon_args.channel = arg_int0("c", "channel", "<channel>", "WiFi channel to monitor");
    _pmon_args.start = arg_lit0(NULL, "start", "start packet monitor");
    _pmon_args.stop = arg_lit0(NULL, "stop", "stop packet monitor");
    _pmon_args.verbose = arg_lit0("v", "verbose", "show statistics in console");
    _pmon_args.end = arg_end(1);
    const esp_console_cmd_t pmon_cmd = {
        .command = "pmon",
        .help = "Monitor all WiFi packets and show statistics",
        .hint = NULL,
        .func = &do_pmon_cmd,
        .argtable = &_pmon_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&pmon_cmd));

    ESP_LOGD(__FUNCTION__, "Started.");
}


esp_err_t CMonitorTask::starting(void) 
{
    lastDrawTime=0;
    pmon_packets.clear();
    return ESP_OK;
}

bool CMonitorTask::execute(void) 
{
    uint32_t currentTime = millis();
    if ( currentTime - lastDrawTime > 1000 ) 
    {
        lastDrawTime = currentTime;
        PACKET_STAT packet;
        packet.pckt_counter=pr_pckt_counter;
        packet.rssi_avg=pr_rssi_avg;
        packet.pr_deauths=pr_deauths;
        pmon_packets.add(packet);
        //draw
        if(f_verbose){
            printf("Ch=%2i Pckts=%-8i rssi=%-8i deauths=%-8i\n", WiFi.get_channel(), pr_pckt_counter, pr_rssi_avg, pr_deauths);
        }
        //////////////////////////////
        //OLED always
        monitor_oled_draw();

        //ALARM
        leds_alarm_set(pr_deauths>=_death_alarm_thresh);

        ////// next round
        pr_deauths = 0;       // deauth frames per second
        pr_pckt_counter = 0;
        pr_rssi_sum = 0;
        lastDrawTime = currentTime;
    }
    return true;
}

esp_err_t CMonitorTask::finished(void) 
{
    WiFi.set_event_handler(NULL); //to avoid recursive calls
    WiFi.set_mode(WiFi_MODE_NONE);
    oled_clear(true);
    return ESP_OK;
} 

esp_err_t CMonitorTask::init(void) 
{ 
    WiFi.set_mode(WiFi_MODE_NONE);
    WiFi.set_promiscuous_callback(pmon_promiscuous_callback);
    WiFi.set_mode(WiFi_MODE_PROMISCUOUS);
    //WiFi.set_event_handler(pmon_events_callback); - tasks maintained via pool
    touchpad_add_handler(TOUCHPAD_EVENT_DOWN, pmon_tpad_onClick);
    return ESP_OK;
}
