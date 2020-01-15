#pragma once

void register_cmd_radar(void);

typedef enum {
  RADAR_PACKETS_ALL=0,
  RADAR_PACKETS_DEAUTH=1,
} RADAR_PACKETS_TYPE;

typedef enum {
  RADAR_OLED_MODE_NONE=0,
  RADAR_OLED_MODE_TIME,
  RADAR_OLED_MODE_HIST,
} RADAR_OLED_MODE;

class CRadarTask: public CTaskThread
{
    public:
        uint32_t           channel; //=0 if all
        mac_t              mac;
        RADAR_PACKETS_TYPE ptype;
        RADAR_OLED_MODE    oledmode;

    protected:
        uint32_t           channel_dur;
        uint32_t           oled_refresh_dur;
        bool               channel_auto;
        uint32_t           starttime;
        uint32_t           channel_starttime;
        uint32_t           stats_starttime;

    protected:
        uint32_t           lastDrawTime;
        bool               oledcleared;

    public:
        CRadarTask(const char* title, uint32_t stack_size=1024*4, uint32_t task_prior=5, CTaskPool* task_pool = NULL) :
            CTaskThread(title, stack_size, task_prior, task_pool)
            {
                channel=0;
                channel_auto=true;
                channel_dur = 100;
                oled_refresh_dur = 500;
                starttime  =0;
                channel_starttime=0;
                oledmode=RADAR_OLED_MODE_NONE;
                oledcleared=true;
            }
        virtual ~CRadarTask(){ }
        void set_oledMode(RADAR_OLED_MODE m);
    protected:
        //CTaskThread
        virtual esp_err_t init(void);
        virtual esp_err_t starting(void);
        virtual bool      execute(void);
        virtual esp_err_t finished(void);
        //
        void reset_stats(void);
    friend int  do_radar_cmd(int argc, char **argv);

};