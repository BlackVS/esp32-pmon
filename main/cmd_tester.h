#pragma once

void register_cmd_deauth(void);

#define DTESTER_PACKETS_DEFAULT 100
#define DTESTER_ARG_PACKETS

class CTesterTask: public CTaskThread
{
    public:
        bool        f_verbose;
        uint32_t    channel;
        uint32_t    packets;

    public:
        //// shared runtime data

    public:
        CTesterTask(const char* title, uint32_t stack_size=1024*4, uint32_t task_prior=5, CTaskPool* task_pool = NULL) :
            CTaskThread(title, stack_size, task_prior, task_pool)
            {
                f_verbose=false;
                channel=0;
                packets=DTESTER_PACKETS_DEFAULT;
            }
        virtual ~CTesterTask(){

        }

        //void update(int32_t i_deauths, int32_t i_pckt_counter, int32_t i_rssi_sum, int32_t i_rssi_avg);

    protected:
        //CTaskThread
        virtual esp_err_t init(void);
        virtual esp_err_t starting(void);
        virtual bool      execute(void);
        virtual esp_err_t finished(void);

};