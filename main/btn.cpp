#include "app.h"

static bool touchpad_enabled = false;
 
static const char *TAG = "TOUCHPAD";
// int is supposed to be atomic
static TOUCHPAD_STATE tp_last_state = TOUCHPAD_STATE_OFF;

TOUCHPAD_EVENT_HANDLER event_handlers[TOUCHPAD_EVENTS];

// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/touch_pad.html
void touchpad_task_init() {
    #ifndef CONFIG_JTAG_ENABLED
    memset(event_handlers, 0, sizeof(event_handlers));
    // Initialize touch pad peripheral.
    // The default fsm mode is software trigger mode.
    touch_pad_init();
    // Set reference voltage for charging/discharging
    // In this case, the high reference valtage will be 2.7V - 1V = 1.7V
    // The low reference voltage will be 0.5
    // The larger the range, the larger the pulse count value.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    //for (int i = 0;i< TOUCH_PAD_MAX;i++) {
    //    touch_pad_config(i, TOUCH_THRESH_NO_USE);
    //}
    touch_pad_config(PIN_TOUCH, TOUCHPAD_THRESH_NO_USE);
    
    #ifdef TOUCHPAD_FILTER_MODE
      touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    #endif
    touchpad_enabled=true;
    #else
    touchpad_enabled=false;
    #endif
  }


void touchpad_add_handler(TOUCHPAD_EVENT eventId, TOUCHPAD_EVENT_HANDLER handler)
{
  if(!touchpad_enabled) 
    return;
  event_handlers[eventId] = handler;
}


void touchpad_task(void *pvParameters) 
{
  if(!touchpad_enabled) {
    vTaskDelete(NULL);
    return;
  }

  #ifndef CONFIG_JTAG_ENABLED

  uint16_t touch_filter_value;
  //ESP_LOGD(TAG, "Starting %s \n", __FUNCTION__);
  LOG_FUNC_BEGIN(TAG)

  esp_err_t res=ESP_OK;

  int duration  = 0;
  
  uint64_t last_click_time = 0;
  bool     last_click_was  = false;
  int      last_click_duration=0;

  uint64_t last_long_time = 0;
  bool     last_long_was  = false;

  while (1) 
  {
    res=touch_pad_read_filtered(PIN_TOUCH, &touch_filter_value);
    if(res!=ESP_OK){
      ESP_LOGE(TAG, "%s\n", esp_err_to_name(res));
      break;
    }
    TOUCHPAD_STATE state = touch_filter_value<=TOUCHPAD_TRESH? TOUCHPAD_STATE_ON : TOUCHPAD_STATE_OFF;
    duration++;
    uint32_t curtime = millis(); //overflow?
    if(state!=tp_last_state)
    {
      //UP / DOWN
      TOUCHPAD_EVENT event = state==TOUCHPAD_STATE_ON ? TOUCHPAD_EVENT_DOWN : TOUCHPAD_EVENT_UP;
      if(event_handlers[event])
        event_handlers[event](event, duration);
      //ESP_LOGD(TAG,"Touchpad event: %i", event);


      //dblclick detection
      if(last_click_was && event==TOUCHPAD_EVENT_DOWN && curtime-last_click_time<TOUCHPAD_DBLCLICK_DELAY){
        if(event_handlers[TOUCHPAD_EVENT_DBLCLICK])
          event_handlers[TOUCHPAD_EVENT_DBLCLICK](TOUCHPAD_EVENT_DBLCLICK, duration);
        last_click_was=false;
        //ESP_LOGD(TAG,"Touchpad event: DBLCLICK");
      } 
      else //delayed click i.e. two separates clicks detection
      {
        if(last_click_was && curtime-last_click_time>=TOUCHPAD_DBLCLICK_DELAY){
          if(event_handlers[TOUCHPAD_EVENT_CLICK])
            event_handlers[TOUCHPAD_EVENT_CLICK](TOUCHPAD_EVENT_CLICK, last_click_duration);
          last_click_was=false;
          //ESP_LOGD(TAG,"Touchpad event: CLICK delayed 1");
        }
        ////
        if(event==TOUCHPAD_EVENT_DOWN){
          last_click_time = curtime;
          last_click_was  = true;
          last_click_duration=duration;        
        }
      }

      //longpress detection start
      if(event==TOUCHPAD_EVENT_DOWN){
        last_long_was  = true;
        last_long_time = curtime;
      }
      /////////////////////////
      duration=0;
      tp_last_state=state;
      //ESP_LOGD(TAG, "event=%i dur=%i\n", event, duration);
    }  
    else  // no state changed - long press detection, delayed click
    {
        if(last_click_was && curtime-last_click_time>=TOUCHPAD_DBLCLICK_DELAY){
          if(event_handlers[TOUCHPAD_EVENT_CLICK]) {
            event_handlers[TOUCHPAD_EVENT_CLICK](TOUCHPAD_EVENT_CLICK, last_click_duration);
            //ESP_LOGD(TAG,"Touchpad event: CLICK delayed 2, event handler call");
          }
          last_click_was=false;
          //ESP_LOGD(TAG,"Touchpad event: CLICK delayed 2");
        }
        if(state==TOUCHPAD_STATE_ON && last_long_was && curtime-last_long_time>=TOUCHPAD_LONGPRESS_DELAY){
          if(event_handlers[TOUCHPAD_EVENT_LONGPRESS])
            event_handlers[TOUCHPAD_EVENT_LONGPRESS](TOUCHPAD_EVENT_LONGPRESS, curtime-last_long_time);
          last_long_time=curtime; 
          //ESP_LOGD(TAG,"Touchpad event: LONG PRESS");
        }
    }
    //ESP_LOGD(TAG, "touch_filter_value=%i\n", touch_filter_value);
    vTaskDelay(5);
  }
  #else
  ESP_LOGW(TAG,"JTAG enabled - touchpad disabled");
  #endif
  LOG_FUNC_END(TAG)
  vTaskDelete(NULL);
}

TOUCHPAD_STATE touchpad_get_state(void)
{
  if(!touchpad_enabled) 
    return TOUCHPAD_STATE_DISABLED;
  return tp_last_state;
}

