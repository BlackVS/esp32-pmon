#pragma once

void register_cmd_scan(void);

class CSnifferTask: public CTask
{
    public:
        uint32_t          duration;
        uint32_t          channel; //=0 if all
        bool              f_verbose;

    protected:
        uint32_t          duration1ch;
        uint32_t          durationTotal;
        bool              channel_auto;
        uint32_t          starttime;
        uint32_t          channel_starttime;

    public:
        CSnifferTask(const char* title, uint32_t stack_size=1024*4, uint32_t task_prior=5, CTaskPool* task_pool = NULL) :
            CTask(title, stack_size, task_prior, task_pool)
            {
                channel=0;
                channel_auto=true;
                duration1ch=0;
                duration   =0;
                starttime  =0;
                channel_starttime=0;
                f_verbose=false;
            }
        virtual ~CSnifferTask(){

        }
    protected:
        virtual esp_err_t init(void);
        //
        virtual esp_err_t starting(void);
        virtual bool      execute(void);
        virtual esp_err_t finished(void);

};