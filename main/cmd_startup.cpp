#include "app.h"

static const char *TAG = __FILE__;

#define _STARTUP_TOKEN "startup"
#define REG_EXTENDED 1
#define REG_ICASE (REG_EXTENDED << 1)


static struct {
    struct arg_rex *cmd;
    struct arg_int *line;
    struct arg_str *text;
    struct arg_end *end;
} _startup_args;


int cmd_startup(int argc, char **argv) 
{
  int nerrors = arg_parse(argc, argv, (void **)&_startup_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, _startup_args.end, argv[0]);
    return 0;
  }

  if(_startup_args.cmd->count==0) {
    printf("Eror: no command set");
    return 0;
  }

  const char* cmd=_startup_args.cmd->sval[0];
  if(strcmp(cmd,"run")==0){
    esp_err_t err=console_script_run(VFS_STARTUP_FILE);
    if(err!=ESP_OK) {
      printf("Startup script failed!\n");
    }
  } else
  if(strcmp(cmd,"clear")==0){
    if( remove(VFS_STARTUP_FILE)==0 ){
      printf("Ok\n");
    } else {
      ESP_LOGD(TAG, "Failed to delete %s : ", VFS_STARTUP_FILE );
      printf("No script file found!\n");
    }
  } else
  if(strcmp(cmd,"show")==0){
    CTextEditor script(VFS_STARTUP_FILE);
    script.print();
  } else
  if(strcmp(cmd,"add")==0){
    if(_startup_args.text->count==0){
      printf("Error: no text specified\n");
      return 0;
    }
    if(_startup_args.text->count!=1){
      printf("Error: only line by line!\n");
      return 0;
    }
    CTextEditor script(VFS_STARTUP_FILE);
    script.add(_startup_args.text->sval[0]);
    script.write();
  } else
  if(strcmp(cmd,"delete")==0){
    if(_startup_args.line->count==0){
      printf("Error: no line number specified\n");
      return 0;
    }
    CTextEditor script(VFS_STARTUP_FILE);
    script.remove(_startup_args.line->ival[0]);
    script.write();
  } else
  if(strcmp(cmd,"edit")==0){
    if(_startup_args.text->count==0){
      printf("Error: no text specified\n");
      return 0;
    }
    if(_startup_args.line->count==0){
      printf("Error: no line number specified\n");
      return 0;
    }
    CTextEditor script(VFS_STARTUP_FILE);
    script.replace(_startup_args.line->ival[0], _startup_args.text->sval[0]);
    script.write();
  } else {
    printf(" %s : not implemented\n", cmd);
  }
  return 0;
}

void register_cmd_startup() {
  _startup_args.cmd  = arg_rex1(NULL, NULL, "clear|show|add|delete|edit|run", NULL, REG_ICASE, NULL);
  _startup_args.line = arg_int0("l", "line", "<line_number>", "Lime number to edit");
  _startup_args.text = arg_str0("t", "text", "<text>", "Text to insert");
  _startup_args.end  = arg_end(1);

  const esp_console_cmd_t cmd = {
      .command = "startup",
      .help = "Startup script editor",
      .hint = NULL,
      .func = &cmd_startup,
      .argtable = &_startup_args
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
