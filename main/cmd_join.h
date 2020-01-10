#pragma once

void register_cmd_join(void);

class CJoinTask: public CTaskStatic
{
    public:
        std::string ssid;
        std::string password;
        uint32_t    timeout;

    public:
        CJoinTask(const char* title, uint32_t stack_size=1024*4, uint32_t task_prior=5, CTaskPool* task_pool = NULL) :
            CTaskStatic(task_pool)
            {
                timeout=15000;
            }
        virtual ~CJoinTask(){

        }
        bool is_connected(void);
        
    protected:
        //CTaskStatic
        virtual esp_err_t _start_internal(void);
        virtual esp_err_t _stop_internal(void);
};

extern CJoinTask wifi_join;