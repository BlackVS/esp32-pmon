#include "app.h"

//static const char *TAG = __FILE__;


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
    }
    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}

