#include "app.h"

#include "ws2812_control.h"

#define LEDS_ALARM_REPEAT 20
static bool leds_alarm_state =false;
static bool leds_alarm_repeat=true;
static uint32_t leds_alarm_R=0;
static uint32_t leds_alarm_G=0;
static uint32_t leds_alarm_B=0;
static uint32_t leds_alarm_num=NUM_LEDS;
static float    leds_alarm_dim=0.4f;

void leds_effect1_init(uint32_t color=LED_GREEN, float fDim=0.4f, uint32_t value=10)
{
  leds_alarm_R=R(color);
  leds_alarm_G=G(color);
  leds_alarm_B=B(color);
  leds_alarm_dim=fDim;
  leds_alarm_num=MIN(10, value);
  for(uint32_t i=0; i<leds_alarm_num; i++)
    leds_set_color_rgb(i, leds_alarm_R, leds_alarm_G, leds_alarm_B);
}

void leds_effect1_iter()
{
  if(!leds_alarm_state)
    return;

  for(uint32_t i=0;i<NUM_LEDS;i++)
  {
    uint32_t color=leds_get_color_raw(i);
    uint32_t cR=R(color);
    uint32_t cG=G(color);
    uint32_t cB=B(color);
    cR=uint32_t(cR*leds_alarm_dim);
    cG=uint32_t(cG*leds_alarm_dim);
    cB=uint32_t(cB*leds_alarm_dim);
    leds_set_color_rgb(i, cR, cG, cB);
  }

  
  if(!leds_alarm_repeat)
    return;

  static uint32_t period=LEDS_ALARM_REPEAT;
  period--;
  if(period==0)
  {
    period=LEDS_ALARM_REPEAT;
    for(uint32_t i=0; i<leds_alarm_num; i++)
      leds_set_color_rgb(i, leds_alarm_R, leds_alarm_G, leds_alarm_B);
  }

}

void leds_alarm_task(void *pvParameters) {
  leds_effect1_init();
  while (1) {
    leds_effect1_iter();
    leds_update();
    vTaskDelay(10);
  }
  vTaskDelete(NULL); 
}

void leds_alarm_set(bool v, uint32_t color, float fDim, uint32_t value, bool bRepeat)
{
  if(v)
    leds_effect1_init(color, fDim, value);
  else 
    leds_clear();
  leds_alarm_repeat=bRepeat;
  leds_alarm_state=v;
}
