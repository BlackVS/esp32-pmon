#include "app.h"

//static const char *TAG = __FILE__;

std::map<uint64_t,wifi_target_t> targets;


const char * target2str(target_t t){
    switch(t){
        case TARGET_AP:
            return "AP";
        case TARGET_STATION:
            return "ST";
        case TARGET_UNKNOWN:
            return "--";
    }
    return "--";
}


void targets_print(void)
{
    size_t cnt=targets.size();
    if(cnt) 
    {
        printf("\nTargets (stations/APs): %i\n", cnt);
        printf("--------------------------------------------------------------------------\n");
        printf(" ID        MAC          CH  Type       MAC_Link      Info\n");
        printf("--------------------------------------------------------------------------\n");
        if(cnt>0) {
            int id=0;
            for(auto& t:targets){
                wifi_target_t& d=t.second;
                printf( " %2i %s   %2d   %s   %s  %s\n", id, mac2str(d.mac).c_str(), d.channel, target2str(d.type), mac2str(d.mac_linked2ap).c_str(), d.desc);
                id++;
            }
        }
        printf("--------------------------------------------------------------------------\n");
    }

}


bool target_exists(mac_t mac)
{
    uint64_t macint = mac.to_int();
    return targets.count(macint)!=0; 
}

void target_add(mac_t mac, target_t type, uint8_t channel, const char* desc, mac_t link)
{
    wifi_target_t t(mac, type, channel, desc, link);
    targets[mac.to_int()]=t;
}

bool target_get(mac_t mac, wifi_target_t& t)
{
    uint64_t macint = mac.to_int();
    if(targets.count(macint)==0)
        return false;
    t=targets[macint];
    return true;
}