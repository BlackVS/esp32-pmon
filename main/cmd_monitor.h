#pragma once

#define CONFIG_PMON_TASK_STACK_SIZE 1024*8
#define CONFIG_PMON_TASK_PRIORITY 5
#define CONFIG_PMON_DEAUTH_DETECT_LEVEL 1

#define PACKET_PROCESS_PACKET_TIMEOUT_MS (100)


///// netif not supports C++ compiler yet
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"
#include "lwip/apps/sntp.h"
#include "lwip/err.h"

#ifdef __cplusplus
}
#endif

// 1 - 14 channels (1-11 for US, 1-13 for EU and 1-14 for Japan)
#define WIFI_MAX_CH 14      
#define WIFI_SNAP_LEN 2324   // max len of each recieved packet



void register_cmd_monitor(void);

class CMonitorTask: public CTaskThread
{
    public:
        bool              f_verbose;

    public:
        //// shared runtime data
        int32_t  pr_deauths;       // deauth frames per second
        int32_t  pr_pckt_counter;
        int32_t  pr_rssi_sum;
        int32_t  pr_rssi_avg;

    protected:
        uint32_t lastDrawTime;

    public:
        CMonitorTask(const char* title, uint32_t stack_size=1024*4, uint32_t task_prior=5, CTaskPool* task_pool = NULL) :
            CTaskThread(title, stack_size, task_prior, task_pool)
            {
                pr_deauths=0; 
                pr_pckt_counter=0;
                pr_rssi_sum=0;
                pr_rssi_avg=0;
                f_verbose=false;
                lastDrawTime=0;
            }
        virtual ~CMonitorTask(){

        }

        //void update(int32_t i_deauths, int32_t i_pckt_counter, int32_t i_rssi_sum, int32_t i_rssi_avg);

    protected:
        //CTaskThread
        virtual esp_err_t init(void);
        virtual esp_err_t starting(void);
        virtual bool      execute(void);
        virtual esp_err_t finished(void);

};