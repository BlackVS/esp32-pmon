#pragma once

extern "C" typedef void (*TASK_CALLBACK)(void *pvParameters);
extern "C" void _task_callback(void *pvParameters);

class ITask
{
    //internal methods
    protected:
        // virtual esp_err_t init(void)    =0;  //pre-thread
        // virtual esp_err_t starting(void)=0;  //in-thread
        // virtual bool      execute(void) =0;  //in-thread
        // virtual esp_err_t finished(void)=0;  //in-thread
    //public methods
    public:
        virtual esp_err_t   start(void)          =0;
        virtual esp_err_t   stop(bool fWait=true)=0;
        virtual bool        is_running(void)     =0;
        virtual esp_err_t   _start_internal(void)=0;//just start task without any additional conditions
};

class CTaskPool
{   
        bool bExclusive; //if true only one task can be active from pool
        std::vector<ITask*> pool;
    public:
        CTaskPool(bool bExcl=true)
        {
            bExclusive=bExcl;
        }

        int32_t   task_add(ITask*);
        esp_err_t task_start(int32_t task_id);
        esp_err_t task_stop(int32_t task_id);
        esp_err_t tasks_stop_all(void);

};

class CTaskStatic: public ITask
{
        CTaskPool*      task_pool;
        bool            task_running;
        int32_t         task_id;

    public:
        CTaskStatic(CTaskPool* pool = NULL)
        {
            task_pool=pool;
            task_running=false;
            task_id=-1;
        }
        virtual ~CTaskStatic(){}

    // ITask
    public:
        virtual esp_err_t start(void);
        virtual esp_err_t stop(bool fWait=true);
        virtual bool      is_running(void) {return task_running;}
        virtual esp_err_t   _start_internal(void);

    //CTaskStatic
    protected:
        virtual esp_err_t   _stop_internal(void);

    protected:
        virtual esp_err_t _add2pool_internal(void);

    friend class CTaskPool;
};

class CTaskThread : public ITask
{
        //
        TASK_CALLBACK   task_func;
        char            task_title[32];
        uint32_t        task_stack_size;
        uint32_t        task_priority;
        CTaskPool*      task_pool;
        int32_t         task_id;
        //
        TaskHandle_t      task_handle;
        SemaphoreHandle_t sem_task_over;
        bool              task_running;

    public:
        CTaskThread(const char* title, uint32_t stack_size=1024*4, uint32_t task_prior=5, CTaskPool* pool = NULL)
        {
            task_func=_task_callback;
            memset(task_title,0,sizeof(task_title));
            strncpy(task_title, title, sizeof(task_title)-1);
            task_stack_size=stack_size;
            task_priority=task_prior;
            task_running=false;
            task_pool=pool;
            task_id = -1;
            //if(task_pool) - defferred adding to pool in "start" method
            //    task_id=task_pool->task_add(this); due can't use "this" in constructor - object not finally constructed yet

            sem_task_over = xSemaphoreCreateBinary();
            xSemaphoreGive(sem_task_over);
        }
        virtual ~CTaskThread(){
            if(sem_task_over){
                vSemaphoreDelete(sem_task_over);
            }
        }

    // ITask
    public:
        virtual esp_err_t start(void);
        virtual esp_err_t stop(bool fWait=true);
        virtual bool      is_running(void) {return task_running;}

    protected:
        virtual esp_err_t _start_internal(void);

    //CTaskThread
    protected:
        virtual esp_err_t init(void) { return ESP_OK;}    //pre-thread
        virtual esp_err_t starting(void) { return ESP_OK;}//in-thread
        virtual bool      execute(void) {return false;}  //in-thread
        virtual esp_err_t finished(void) {return ESP_OK;} //in-thread

    protected:
        virtual esp_err_t _add2pool_internal(void);


    friend void _task_callback(void *pvParameters);
    friend class CTaskPool;
};

