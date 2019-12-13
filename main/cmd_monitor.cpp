#include "app.h"

#include "nano_engine.h"

static const char *TAG = __FILE__;

static int _death_alarm_thresh=10;

typedef struct _pmon_runtime_struct {
    uint32_t magic;
    bool is_running;
    //// args
    bool     verbose;
    //// shared runtime data
    int32_t  pr_deauths;       // deauth frames per second
    int32_t  pr_pckt_counter;
    int32_t  pr_rssi_sum;
    int32_t  pr_rssi_avg;
    //// task 
    TaskHandle_t task;
    SemaphoreHandle_t sem_task_over;

    _pmon_runtime_struct():
        pr_deauths(0), 
        pr_pckt_counter(0),
        pr_rssi_sum(0),
        pr_rssi_avg(0)
    {
        magic=0XBEEFFEEB;
        is_running=false;
        verbose=true;

    }
} pmon_runtime_t;

static pmon_runtime_t _pmon_rt;

static struct {
    struct arg_lit *verbose;
    struct arg_int *channel;
    struct arg_lit *start;
    struct arg_lit *stop;
    struct arg_end *end;
} _pmon_args;

#define PMON_BUFFER_LEN 128

typedef struct {
    uint32_t pckt_counter;
    uint32_t rssi_avg;
    uint32_t pr_deauths;
} PMON_PACKET;

class CBuffer
{
    PMON_PACKET buffer[PMON_BUFFER_LEN];
    uint32_t buflen;
    uint32_t bufpos;
    uint32_t buflen_used;
public:
    CBuffer():
        bufpos(0)
        {
            clear();
        }
    void clear(void){
        bufpos=0;
        buflen_used=0;
        memset(buffer,0,sizeof(buffer));
    }
    void add(PMON_PACKET v){
        //printf("+ %i \n", v);
        while(bufpos>=PMON_BUFFER_LEN)
            bufpos-=PMON_BUFFER_LEN;
        buffer[bufpos]=v;
        bufpos++;
        buflen_used=MIN(buflen_used+1, PMON_BUFFER_LEN);
    }
    uint32_t len(void){
        return PMON_BUFFER_LEN;
    }
    PMON_PACKET get(int32_t pos){
        pos+=bufpos;
        while(pos>=PMON_BUFFER_LEN)
            pos-=PMON_BUFFER_LEN;
        while(pos<0)
            pos+=PMON_BUFFER_LEN;
        return buffer[pos];
    }
    uint32_t get_max_pckt_counter(void){
        uint32_t res=this->get(0).pckt_counter;
        for(int i=1; i<this->len(); i++)
            res=MAX(res, this->get(i).pckt_counter);
        return res;
    }
    uint32_t get_avg_pckt_counter(void){
        if(buflen_used==0)
            return 0;
        uint64_t res=get(-1).pckt_counter;
        for(int j=1; j<buflen_used; j++)
            res+=this->get(-1-j).pckt_counter;
        return res/buflen_used;
    }

} pmon_packets;

///////////////////////////////////////////////////////////////////////////////////
//
//
extern "C" void pmon_promiscuous_callback(void* buf, wifi_promiscuous_pkt_type_t type) 
{
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

  if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0 )) 
    _pmon_rt.pr_deauths++;

  if (type == WIFI_PKT_MISC) 
    return;             // wrong packet type
  
  _pmon_rt.pr_pckt_counter++;
  _pmon_rt.pr_rssi_sum += ctrl.rssi;
  if(_pmon_rt.pr_pckt_counter){
      _pmon_rt.pr_rssi_avg=_pmon_rt.pr_rssi_sum/_pmon_rt.pr_pckt_counter;
  }

  //printf(".");
}

/// OLED
NanoEngine<TILE_128x64_MONO> engine;

void setup_oled_pmon()
{
    ssd1306_clearScreen();
    engine.begin();
    engine.setFrameRate( 1 );
    engine.canvas.setMode(CANVAS_TEXT_WRAP);
    engine.refresh();
}

void oled_draw()
{
    //if (!engine.nextFrame()) 
    //    return;
    engine.canvas.clear();

    char buf[16];

    itoa(WiFi.get_channel(),buf,10);
    engine.canvas.printFixed(0,  0, "CH:", STYLE_NORMAL);
    engine.canvas.printFixed(24, 0, buf  , STYLE_BOLD);

    itoa(_pmon_rt.pr_rssi_avg,buf,10);
    engine.canvas.printFixed(0,  56, "LV:", STYLE_NORMAL);
    engine.canvas.printFixed(16, 56, buf  , STYLE_BOLD);

    itoa(_pmon_rt.pr_pckt_counter,buf,10);
    engine.canvas.printFixed(96,  0, "P:", STYLE_NORMAL);
    engine.canvas.printFixed(112, 0, buf  , STYLE_BOLD);

    itoa(_pmon_rt.pr_deauths,buf,10);
    engine.canvas.printFixed(96,  56, "D:", STYLE_NORMAL);
    engine.canvas.printFixed(112, 56, buf  , STYLE_BOLD);

    engine.canvas.drawHLine(0,9,127);
    engine.canvas.drawHLine(0,53,127);

    uint32_t yt = 11;
    uint32_t yb = 51;

    uint32_t vmax=pmon_packets.get_max_pckt_counter();
    itoa(vmax,buf,10);
    engine.canvas.printFixed(58, 0, buf, STYLE_BOLD);

    uint32_t vavg=pmon_packets.get_avg_pckt_counter();
    itoa(vavg,buf,10);
    engine.canvas.printFixed(58, 56, buf, STYLE_BOLD);

    if(vmax>0){
        for(int i=0; i<pmon_packets.len(); i++ ){
            uint32_t v  = pmon_packets.get(i).pckt_counter;
            uint32_t dy = (v*(yb-yt))/vmax;
            engine.canvas.drawVLine(i, yb-dy, yb);
            //printf("%i %i %i %i %i\n", i, v, dy, yb-dy, yb);
        }
    }
    engine.display();
}

///////////////////////////////////////////////////////////////////////////////////
//
//
extern "C" void pmon_task(void *pvParameters) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    if(pvParameters!=&_pmon_rt){
        ESP_LOGE(TAG,"Failed to start pmon task - not  arg");
        vTaskDelete(NULL);
        return;
    }
    pmon_runtime_t *pmon = (pmon_runtime_t *)pvParameters;
    if(!pmon){
        ESP_LOGE(TAG,"Failed to start pmon task - invalid arg");
        vTaskDelete(NULL);
        return;
    }

    uint32_t lastDrawTime=0;
    pmon_packets.clear();

    setup_oled_pmon();

    while (pmon->is_running) 
    {
        
        uint32_t currentTime = millis();
        if ( currentTime - lastDrawTime > 1000 ) 
        {
            lastDrawTime = currentTime;
            PMON_PACKET packet;
            packet.pckt_counter=_pmon_rt.pr_pckt_counter;
            packet.rssi_avg=_pmon_rt.pr_rssi_avg;
            packet.pr_deauths=_pmon_rt.pr_deauths;
            pmon_packets.add(packet);
            //draw
            if(pmon->verbose){
                printf("Ch=%2i Pckts=%-8i rssi=%-8i deauths=%-8i\n", WiFi.get_channel(), _pmon_rt.pr_pckt_counter, _pmon_rt.pr_rssi_avg, _pmon_rt.pr_deauths);
            }
            //////////////////////////////
            //OLED always
            oled_draw();

            //ALARM
            leds_alarm_set(_pmon_rt.pr_deauths>_death_alarm_thresh);

            ////// next round
            pmon->pr_deauths = 0;       // deauth frames per second
            pmon->pr_pckt_counter = 0;
            pmon->pr_rssi_sum = 0;
            lastDrawTime = currentTime;
        }
        //allow doing rest jobs
        vTaskDelay(10);
    }
    
    /* notify that sniffer task is over */
    ESP_LOGD(__FUNCTION__, "Left");
    xSemaphoreGive(pmon->sem_task_over);
    vTaskDelete(NULL); //self delete
}


// click - change channel
void pmon_tpad_onClick(TOUCHPAD_EVENT, int value)
{
    WiFi.set_channel( WiFi.get_channel()+1 );
    pmon_packets.clear();
}


///////////////////////////////////////////////////////////////////////////////////
//
//
static esp_err_t pmon_stop(pmon_runtime_t *pmon)
{
    ESP_LOGD(__FUNCTION__, "Stopping task");
    if(!pmon->is_running){
        ESP_LOGW(TAG, "Packet monitor already stopped");
        return ESP_OK; //pass OK due to it is not critical
    }

    WiFi.set_mode(WiFi_MODE_NONE);

    pmon->is_running = false; //trigger to stop
    //wait until finished
    xSemaphoreTake(pmon->sem_task_over, portMAX_DELAY);
    vSemaphoreDelete(pmon->sem_task_over);
    pmon->sem_task_over = NULL;
    pmon->task=NULL;
    WiFi.set_event_handler(NULL);
    ssd1306_clearScreen();
    ESP_LOGD(__FUNCTION__, "Stopped OK.");
    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////////
//
//
void pmon_events_callback(WiFi_EVENT ev, uint32_t arg)
{
    ESP_LOGD(__FUNCTION__, "Event: %i", ev);
    if(ev==WiFi_EVENT_MODE_CHANGED){
        ESP_LOGD(__FUNCTION__, "Event: WiFi_EVENT_MODE_CHANGED");
        WiFi_MODES mode = (WiFi_MODES)arg;
        if(_pmon_rt.is_running && mode!=WiFi_MODE_PROMISCUOUS)
        {
            ESP_LOGD(__FUNCTION__, "Event: gracefully shutdown");
            //gracefully shutdown
            pmon_stop(&_pmon_rt);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////
//
//
static esp_err_t pmon_start(pmon_runtime_t *pmon)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    if(pmon->is_running){
        ESP_LOGW(TAG, "Packet monitor already started");
        return ESP_OK; //pass OK due to it is not critical
    }

    WiFi.set_mode(WiFi_MODE_NONE);
    WiFi.set_promiscuous_callback(pmon_promiscuous_callback);
    WiFi.set_mode(WiFi_MODE_PROMISCUOUS);
    WiFi.set_event_handler(pmon_events_callback);

    pmon->sem_task_over = xSemaphoreCreateBinary();
    BaseType_t ret = xTaskCreate( pmon_task, 
                                 "pmonTask", 
                                CONFIG_PMON_TASK_STACK_SIZE,
                                pmon, 
                                CONFIG_PMON_TASK_PRIORITY, 
                                &pmon->task);
    if(ret != pdTRUE)
    {
        ESP_LOGD(__FUNCTION__, "FAILED to start!!!");
        WiFi.set_mode(WiFi_MODE_NONE);
        pmon->task=NULL;
        pmon->is_running = false;
        return ESP_FAIL;
    }

    //touchpad_add_handler(TOUCHPAD_EVENT_CLICK, pmon_tpad_onClick);
    touchpad_add_handler(TOUCHPAD_EVENT_DOWN, pmon_tpad_onClick);

    pmon->is_running = true;
    ESP_LOGD(__FUNCTION__, "Started OK.");
    return ESP_OK;
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

    bool bChangeOnTheFly = _pmon_rt.is_running && !_pmon_args.start->count && !_pmon_args.stop->count;
    // --stop 
    if (_pmon_args.stop->count) {
        pmon_stop(&_pmon_rt);
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
    _pmon_rt.verbose = _pmon_args.verbose->count > 0;

    // --start
    if (_pmon_args.start->count) {
        pmon_start(&_pmon_rt);
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
