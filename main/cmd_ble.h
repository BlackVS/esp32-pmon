#pragma once


//#include <bt_types.h>
//#include <freertos/FreeRTOS.h>
//#include <freertos/task.h>
//#include <ctype.h>
//#include <string.h>
//#include <math.h>
//#include "nvs_flash.h"
//#include "sdkconfig.h"


void register_cmd_ble(void);

class CBleTask: public CTaskThread
{
    public:
        uint32_t          duration;
        bool              f_verbose;

    protected:
        uint32_t          starttime;
        //bool              f_ble_inited;
        //bool              f_ble_failed;

    public:
        CBleTask(const char* title, uint32_t stack_size=1024*2, uint32_t task_prior=5, CTaskPool* task_pool = NULL) :
            CTaskThread(title, stack_size, task_prior, task_pool)
            {
                duration   =0;
                starttime  =0;
                f_verbose=false;
                task_delay = 1000;
                //f_ble_inited=false;
                //f_ble_failed=false;
            }
        virtual ~CBleTask(){

        }
    protected:
        //CTaskThread
        virtual esp_err_t init(void);
        virtual esp_err_t starting(void);
        virtual bool      execute(void);
        virtual esp_err_t finished(void);

};

//void bt_task(void *ignore);