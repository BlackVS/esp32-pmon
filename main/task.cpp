#include "app.h"

static const char *TAG = __FILE__;

extern "C" void _task_callback(void *pvParameters) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    CTaskThread* task=(CTaskThread*)pvParameters;

    if(!task){
        ESP_LOGE(__FUNCTION__, "Error: no task given to callback");
        vTaskDelete(NULL); //self delete
    }

    task->starting();
    
    if(task->sem_task_over)
        xSemaphoreTake(task->sem_task_over,portMAX_DELAY);
    task->task_running=true;

    while (task->is_running()) 
    {
        if(!task->execute())
        {
            break;
        }
        vTaskDelay(10);//via params
    }
    task->finished();

    task->task_running=false;
    if(task->sem_task_over)
        xSemaphoreGive(task->sem_task_over);

    ESP_LOGD(__FUNCTION__, "Left");
    vTaskDelete(NULL); //self delete
}

esp_err_t CTaskThread::_start_internal(void)
{
    if(task_running){
        ESP_LOGW(TAG, "Task already running");
        return ESP_OK;
    }
    init();
    BaseType_t ret = xTaskCreate( task_func, 
                                  task_title, 
                                  task_stack_size,
                                  this, 
                                  task_priority, 
                                  &task_handle);
    if(ret != pdTRUE)
    {
        task_handle=NULL;
        ESP_LOGD(__FUNCTION__, "FAILED to start!!!");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t CTaskThread::start(void)
{
    _add2pool_internal(); //deffered adding to pool

    if(task_pool) {
        return task_pool->task_start(task_id);
    }
    return _start_internal();
}

esp_err_t CTaskThread::stop(bool fWait)
{
    if(!task_running) 
        return ESP_OK; //already stopped
    task_running=false;
    if(fWait) {
        xSemaphoreTake(sem_task_over, portMAX_DELAY);
        xSemaphoreGive(sem_task_over);
    }
    return ESP_OK;
}


esp_err_t CTaskThread::_add2pool_internal(void)
{
    if(task_id<0 && task_pool)
        task_id=task_pool->task_add(this);
    return ESP_OK;
}




int32_t  CTaskPool::task_add(ITask* task)
{
    int32_t id=pool.size();
    pool.push_back(task);
    //printf("Pool size is %i\n", (int)pool.size() );
    return id;
}

esp_err_t CTaskPool::task_start(int32_t task_id)
{
    //printf("Pool size is %i\n", (int)pool.size() );
    if(task_id<0 || task_id>=pool.size())
        return ESP_ERR_INVALID_ARG;

    if(bExclusive) {
        for(int i=0; i<pool.size(); i++)
        {
            if(i==task_id) 
                continue;
            ITask* task=pool[i];
            if(task->is_running()){
                task->stop(true);
            }
        }
    }
    ITask* task=pool[task_id];
    if(task->is_running())
    {
        ESP_LOGW(TAG, "Task already running!");
        return ESP_ERR_INVALID_ARG;
    }   

    return task->_start_internal();
}

esp_err_t CTaskPool::task_stop(int32_t task_id)
{
    if(task_id<0 || task_id>=pool.size())
        return ESP_ERR_INVALID_ARG;
    ITask* task=pool[task_id];
    if(!task->is_running())
    {
        ESP_LOGW(TAG, "Task not running!");
        return ESP_ERR_INVALID_ARG;
    }   
    return task->stop(true);
}

esp_err_t CTaskPool::tasks_stop_all(void)
{
    for(int i=0; i<pool.size(); i++)
    {
        ITask* task=pool[i];
        if(task->is_running()){
            task->stop(true);
        }
    }
    return ESP_OK;
}














esp_err_t CTaskStatic::_start_internal(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t CTaskStatic::_stop_internal(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t CTaskStatic::start(void)
{
    _add2pool_internal(); //deffered adding to pool

    esp_err_t err;
    if(task_pool) {
          pool_wifi_tasks.tasks_stop_all();
    }
    err = _start_internal();
    task_running=err==ESP_OK;
    if(!task_running)
        _stop_internal();
    return err;
}

esp_err_t CTaskStatic::stop(bool fWait)
{
    if(!task_running) 
        return ESP_OK; //already stopped
    task_running=false;
    _stop_internal();
    return ESP_OK;
}


esp_err_t CTaskStatic::_add2pool_internal(void)
{
    if(task_id<0 && task_pool)
        task_id=task_pool->task_add(this);
    return ESP_OK;
}
