#pragma once

#define CONFIG_PMON_TASK_STACK_SIZE 1024*8
#define CONFIG_PMON_TASK_PRIORITY 5
#define CONFIG_PMON_DEAUTH_DETECT_LEVEL 1

#define PACKET_PROCESS_PACKET_TIMEOUT_MS (100)


void register_cmd_monitor(void);

#define PMON_DEFAULT_BUFFER_LEN 128

typedef struct {
    uint32_t pckt_counter;
    uint32_t pr_deauths;
    int32_t  rssi_avg;
} PACKET_STAT;

class CPacketsBuffer
{
    std::vector<PACKET_STAT> buffer;
    int32_t bufpos;
    int32_t buflen_used;
public:
    CPacketsBuffer(int buflen):
        bufpos(0)
        {
            buffer.resize(buflen);
            clear();
        }

    void clear(void){
        bufpos=0;
        buflen_used=0;
        for(auto& b: buffer)
            memset(&b,0,sizeof(b));
    }
    void add(PACKET_STAT v){
        //printf("+ %i \n", v);
        int32_t buflen=buffer.size();
        while(bufpos>=buflen)
            bufpos-=buflen;
        buffer[bufpos]=v;
        bufpos++;
        buflen_used=MIN(buflen_used+1, buflen);
    }
    
    uint32_t len(void){
        return buffer.size();
    }

    PACKET_STAT get(int32_t pos){
        pos+=bufpos;
        int32_t buflen=buffer.size();
        while(pos>=buflen)
            pos-=buflen;
        while(pos<0)
            pos+=buflen;
        return buffer[pos];
    }

    uint32_t get_max_pckt_counter(void){
        uint32_t res=this->get(0).pckt_counter;
        int32_t buflen=buffer.size();
        for(int i=1; i<buflen; i++)
            res=MAX(res, this->get(i).pckt_counter);
        return res;
    }

    uint32_t get_avg_pckt_counter(void){
        if(buflen_used==0)
            return 0;
        uint64_t res=get(-1).pckt_counter;
        for(int j=1; j<buflen_used; j++)
            res+=this->get(-1-j).pckt_counter;
        return res/buflen_used;
    }

    int32_t get_avg_pckt_rssi(void){
        if(buflen_used==0)
            return 0;
        int64_t res=get(-1).rssi_avg;
        for(int j=1; j<buflen_used; j++)
            res+=get(-1-j).rssi_avg;
        return res/buflen_used;
    }

    int32_t get_max_pckt_rssi(void){
        int32_t res=-100;//def zero level
        int32_t buflen=buffer.size();
        for(int i=0; i<buflen; i++) {
            int32_t v=get(i).rssi_avg;
            if(v==0) continue;
            res=MAX(res, v);
        }
            
        return res;
    }

    int32_t get_min_pckt_rssi(void){
        int32_t res=0;
        int32_t buflen=buffer.size();
        for(int i=0; i<buflen; i++) {
            int32_t v=get(i).rssi_avg;
            if(v==0) continue;
            res=MIN(res, v);
        }
        return res;
    }

};


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