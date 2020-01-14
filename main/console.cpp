#include "app.h"

//static const char *TAG = __FILE__;



//////////////////////////////////////////////////////////////////////////////////////
//
//
void console_init(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");


    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);


    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_length = 256,
            .max_cmdline_args = 8,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN),
#endif
            .hint_bold = false
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */ 
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

#if CONFIG_STORE_HISTORY
    linenoiseHistorySetMaxLen(20);
     /* Load command history from filesystem */
    int res = linenoiseHistoryLoad(CONSOLE_HISTORY_PATH);
    if(res==0){
         ESP_LOGI(__FUNCTION__,"History succesfully loaded or file not exists");
     } else {
         ESP_LOGW(__FUNCTION__,"Failed to load history");
     }
#endif
  ESP_LOGD(__FUNCTION__, "Started...");
}

//////////////////////////////////////////////////////////////////////////////////////
//
//
void register_cmds() 
{
  register_cmd_system();
  register_cmd_join();
  register_cmd_npm();
  register_cmd_targets();
  register_cmd_monitor();
  register_cmd_deauth();
  register_cmd_scan();
  register_cmd_radar();
}


esp_err_t console_script_run(const char * filename) 
{
    CTextEditor script(filename);
    if(!script.isLoaded()){
        printf("No script file %s found!\n", filename);
        return ESP_ERR_INVALID_ARG;
    }
    if(script.size()==0) {
        printf("Script file is empty!\n");
        return ESP_ERR_INVALID_ARG;
    }
    printf("Startup script is found!\n");
    for(int i=0; i<script.size(); i++)
    {
        int ret;
        std::string line=script.get(i);
        printf("LINE %i: %s\n", i, line.c_str());
        esp_err_t err=esp_console_run(line.c_str(), &ret);
        if(err==ESP_OK) {
            printf("OK\n");
        } else {
            printf("Error: %s\n", esp_err_to_name(err));   
            printf("Script aborted!\n");   
            return err;
        }
    }
    return ESP_OK;
}

//////////////////////////////////////////////////////////////////////////////////////
//
//
//void console_task(void *param) 
void console_task(const char* startcmd, bool bStartupEnable) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");

    /* Register commands */
    esp_console_register_help_command();
    register_cmds();

    const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

    printf(LOG_COLOR_W
         "\n\n<<< BadgeOS %s >>>\n"
         "Type 'help' to get the list of commands.\n"
         "Use UP/DOWN arrows to navigate through command history\n"
         "Press TAB when typing command name to auto-complete.\n"
         "+-------------------------------------------------------+\n"
         "\n" LOG_RESET_COLOR,
         NNC_FW_VERSION);

    /* Figure out if the terminal supports escape sequences */
    int ret;
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
            "Your terminal application does not support escape sequences.\n"
            "Line editing and history features are disabled.\n"
            "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
    #if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
            * don't use color codes in the prompt.
            */
        prompt = "esp32> ";
    #endif //CONFIG_LOG_COLORS
    }

    if(startcmd){
        /*esp_err_t err =*/ esp_console_run(startcmd, &ret);
    }
    if(bStartupEnable)
    {
        console_script_run(VFS_STARTUP_FILE);
    }
    /* Main loop */
    while (true) {
        /* Get a line using linenoise.
        * The line is returned when ENTER is pressed.
        */
        char *line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }

        linenoiseHistoryAdd(line);
        #if CONFIG_STORE_HISTORY
        linenoiseHistorySave(CONSOLE_HISTORY_PATH);
        #endif

        /* Try to run the command */
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            printf("Empty command\n");
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
        line=NULL;
        vTaskDelay(0);
    }
    ESP_LOGD(__FUNCTION__, "Started...");
}
