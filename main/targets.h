#pragma once

typedef enum {
    TARGET_UNKNOWN=0,
    TARGET_AP,
    TARGET_STATION 
} target_t;

typedef struct _wifi_target_struct {
    mac_t       mac;
    target_t    type;
    uint8_t     channel;
    char        desc[19]; // 19 due to make sizeof(struct)==32

    _wifi_target_struct(){
        //mac has own constructor
        type=TARGET_UNKNOWN;
        channel=0;
        memset(&desc,0,sizeof(desc));
    }
    _wifi_target_struct(mac_t m, target_t t, uint8_t c, const char* d){
        mac=m;
        type=t;
        channel=c;
        if(d)
            strncpy(desc, d, sizeof(desc)-1);
        else
            memset(&desc,0,sizeof(desc));
    }

} wifi_target_t;