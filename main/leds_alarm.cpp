#include "app.h"

#include "ws2812_control.h"

static bool leds_alarm_state=false;

typedef struct {
  uint32_t cr, cg, cb;
  uint32_t iR, iG, iB;
  float f_dim;
} effect1_DATA;

static effect1_DATA alarm_data;

void leds_effect1_init(uint32_t color=LED_RED, float fDim=0.4f)
{
  alarm_data.cb=alarm_data.cg=alarm_data.cr=0;
  alarm_data.iB=B(color);
  alarm_data.iG=G(color);
  alarm_data.iR=R(color);  
  alarm_data.f_dim=fDim;
}

void leds_effect1_draw(effect1_DATA& data)
{
  leds_clear();
  if(leds_alarm_state)
  {
    for(int i=0;i<NUM_LEDS;i++)
    {
      leds_set_color_rgb(i,data.cr,data.cg,data.cb);
    }
  }
}

void leds_effect1_iter(effect1_DATA& data)
{
  data.cr=uint32_t(data.cr*data.f_dim);
  data.cg=uint32_t(data.cg*data.f_dim);
  data.cb=uint32_t(data.cb*data.f_dim);
  if( data.cr==0 && data.cg==0 && data.cb==0 )
  {
    data.cr=data.iR;
    data.cg=data.iG;
    data.cb=data.iB;
  }
}

void leds_alarm_task(void *pvParameters) {
  leds_effect1_init();
  while (1) {
    leds_effect1_draw(alarm_data);
    leds_effect1_iter(alarm_data);
    leds_update();
    vTaskDelay(10);
  }
  vTaskDelete(NULL); 
}

void leds_alarm_set(bool v, uint32_t color, float fDim)
{
  if(v)
    leds_effect1_init(color, fDim);
  leds_alarm_state=v;
}
