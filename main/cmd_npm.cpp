#include "app.h"


static const char *TAG = __FILE__;

typedef struct {
  std::string name;
  std::string title;
  std::string author;
  std::string version;
  std::string date;
  std::string source;
  std::string description;
  //
  std::string bin_url;
} NPM_PACKET;

std::vector<NPM_PACKET> packets;
typedef std::vector<std::string> ARRSTR;

static struct {
    struct arg_lit *list;
    struct arg_lit *verbose;
    struct arg_int *install;
    struct arg_end *end;
} _npm_args;


std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}
 
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    return ltrim(rtrim(str, chars), chars);
}

int npm_split(const std::string& s, ARRSTR& data)
{
    size_t pos=0;
    size_t len=s.size();
    std::string t;
    data.clear();
    while(pos<len){
      char c=s[pos];
      if(c=='\n'||c=='\r'){
        if(t.size()>1&&t[0]!='#')
          data.push_back(t);
        t.clear();
        pos++;
        //skip
        while(pos<len&&(s[pos]=='\n'||s[pos]=='\r'))
          pos++;
      } else 
      {
        t+=c;
        pos++;
      }
    }
    return data.size();
}

esp_err_t npm_parse(const std::string& data, NPM_PACKET& res)
{
  ARRSTR text;
  int cnt=npm_split(data, text);
  if(cnt<7){
    printf("Wrong packets info file!\n");
    return ESP_ERR_INVALID_ARG;
  }
  for(auto& s: text){
    //split by :
    size_t pd=s.find(':');
    if(pd==std::string::npos) {
      printf("Wrong packets info file - no delimiter found!\n");
      return ESP_ERR_INVALID_ARG;
    }
    std::string key  =s.substr(0,pd); trim(key);
    std::string value=s.substr(pd+1); trim(value);
    ESP_LOGD(TAG, "[%s]=[%s]", key.c_str(), value.c_str());
    if(key.compare("NAME")==0){
      res.name=value;
    } else
    if(key.compare("TITLE")==0) {
      res.title=value;
    } else
    if(key.compare("AUTHOR")==0){
      res.author=value;
    } else
    if(key.compare("VERSION")==0){
      res.version=value;
    } else
    if(key.compare("DATE")==0){
      res.date=value;
    } else
    if(key.compare("SOURCE")==0){
      res.source=value;
    } else
    if(key.compare("DESCRIPTION")==0){
      res.description=value;
    } else {
      ESP_LOGD(TAG, "Unknown key %s", key.c_str());
    }
   }
  return ESP_OK;
}





esp_err_t npm_load_list(void)
{
  packets.clear();
  std::string data;
  esp_err_t err=http_get(NPM_REPOSITORY_URL, data);
  if(err!=ESP_OK){
    ESP_LOGE(TAG, "Failed to connect to repository!");
    return err;
  }
  //printf("Read %i bytes\n", (int)data.size());
  ARRSTR purls;
  int pcnt=npm_split(data,purls);
  if(pcnt){
    //printf("%i packets available:\n",pcnt);
    for(int i=0;i<pcnt;i++){
      //printf("%i: %s\n",i,purls[i].c_str());
      std::string purl=purls[i];
      //1. read title
      std::string ptitle=purl+"title.txt";
      err=http_get( ptitle.c_str(), data);
      if(err!=ESP_OK){
        ESP_LOGE(TAG,"Failed to get %s", purl.c_str());
        continue;
      }
      NPM_PACKET pckt;
      err=npm_parse(data, pckt);
      if(err!=ESP_OK) {
        ESP_LOGE(TAG,"Failed to parse packet info");
        continue;
      }
      pckt.bin_url=purl+"bin/firmware.bin";
      packets.push_back(pckt);
    }
  }
  return ESP_OK;
}

void npm_print_list(bool bDetailed=false)
{
  if(packets.size()==0) {
    printf("No packets vailable\n");
  }
  printf("There are %i packets in repository:\n", (int)packets.size());
  if(!bDetailed) {
    printf("------------------------------------------------------------\n");
    printf("ID  Name             Title                                  \n");
    printf("------------------------------------------------------------\n");
  } else {
    printf("------------------------------------------------------------\n");
  }
  for(int i=0;i<packets.size();i++){
    if(bDetailed) {
      printf("ID     : %i\n", i);
      printf("NAME   : %s\n", packets[i].name.c_str());
      printf("TITLE  : %s\n", packets[i].title.c_str());
      printf("AUTHOR : %s\n", packets[i].author.c_str());
      printf("VERSION: %s\n", packets[i].version.c_str());
      printf("DATE   : %s\n", packets[i].date.c_str());
      printf("SOURCE : %s\n", packets[i].source.c_str());
      printf("BIN_URL: %s\n", packets[i].bin_url.c_str());
      printf("DESCRIPTION:  %s\n", packets[i].description.c_str());
      printf("------------------------------------------------------------\n");
    } else {
      printf("%2i  %-16s %s\n",i, packets[i].name.c_str(), packets[i].title.c_str());
    }
  }
}

/* 'version' command */
int cmd_npm(int argc, char **argv) 
{
  int nerrors = arg_parse(argc, argv, (void **)&_npm_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, _npm_args.end, argv[0]);
    return 0;
  }

  bool bList   =_npm_args.list->count > 0;
  bool bInstall=_npm_args.install->count > 0;
  if(!bList&&!bInstall)
    bList=true;
  if(bList&&bInstall){
    printf("Please use only one command at the same time!\n");
    return 0;
  }

  if(!wifi_join.is_connected()){
    printf("Please connect to Internet first!\n");
    return 0;
  }
  
  if(bList) {
    if(npm_load_list()==ESP_OK){
      npm_print_list(_npm_args.verbose->count > 0);
    } else {
      printf("Failed to load packets list!!!\n");
    }
  }
  return 0;
}

void register_cmd_npm() {
  _npm_args.list = arg_lit0("l", "list", "List available packets");
  _npm_args.verbose = arg_lit0("v", "verbose", "Verbose/detailed output");
  _npm_args.install = arg_int0("i", "install", "<packet_id", "Install packet");
  _npm_args.end = arg_end(1);

  const esp_console_cmd_t cmd = {
      .command = "npm",
      .help = "NoName badge Packets Manager",
      .hint = NULL,
      .func = &cmd_npm,
      .argtable = &_npm_args
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
