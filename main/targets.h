#pragma once

typedef enum {
    TARGET_UNKNOWN=0,
    TARGET_AP,
    TARGET_STATION 
} target_t;

typedef struct _wifi_target_struct {
    mac_t mac;           //mac of target AP or Station
    mac_t mac_linked2ap; //mac of AP to which target station talks

    target_t    type;
    uint8_t     channel;
    char        desc[34]; // 33 - max BSSID len

    _wifi_target_struct(){
        //mac has own constructor
        type=TARGET_UNKNOWN;
        channel=0;
        memset(&desc,0,sizeof(desc));
    }
    _wifi_target_struct(mac_t m, target_t t, uint8_t c, const char* d, mac_t link){
        mac=m;
        type=t;
        channel=c;
        mac_linked2ap=link;
        if(d)
            strncpy(desc, d, sizeof(desc)-1);
        else
            memset(&desc,0,sizeof(desc));
    }

} wifi_target_t;

extern std::map<uint64_t,wifi_target_t> targets;

void targets_print(void);
bool target_exists(mac_t mac);
void target_add(mac_t mac, target_t type, uint8_t channel, const char* desc, mac_t link);
bool target_get(mac_t, wifi_target_t& t);