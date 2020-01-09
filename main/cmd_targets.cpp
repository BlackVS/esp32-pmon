#include "app.h"
#include <map>

//static const char *TAG = "TARGETS";

static struct {
    struct arg_lit *list;
    struct arg_lit *clear;
    struct arg_int *remove;
    struct arg_end *end;
} _targets_args;



///////////////////////////////////////////////////////////////////////////////////
//
//
static int do_targets_cmd(int argc, char **argv)
{
    ESP_LOGD(__FUNCTION__, "Enter...");
    int nerrors = arg_parse(argc, argv, (void **)&_targets_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, _targets_args.end, argv[0]);
        return 0;
    }

    if (_targets_args.list->count > 0){
        targets_print();
        return 0;
    }

    if (_targets_args.clear->count > 0){
        //show current or last scan results, rest parameters just ignored
        return 0;
    }

    // --delete
    if (_targets_args.remove->count) {
        for(int i=0;i<_targets_args.remove->count;i++)
        {
            //printf(_targets_args.remove->ival[i]);
        }
    }
    ESP_LOGD(__FUNCTION__, "Left.");
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////
//
//
void register_cmd_targets(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    _targets_args.list = arg_lit0("l", "list", "Show found targets");
    _targets_args.clear = arg_lit0("c", "clear", "Clear");
    _targets_args.remove = arg_intn("d", "delete", "<id>", 0, 16, "Remove AP/stations from list by id");
    _targets_args.end = arg_end(1);
    const esp_console_cmd_t targets_cmd = {
        .command = "targets",
        .help = "Show found APs/Stations",
        .hint = NULL,
        .func = &do_targets_cmd,
        .argtable = &_targets_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&targets_cmd));
    ESP_LOGD(__FUNCTION__, "Started.");
}




