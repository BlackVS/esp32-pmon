#include "app.h"

#include "ws2812_control.h"

static bool leds_alarm_state=false;

typedef struct {
  uint32_t r, g, b;
  float f_dim;
} effect1_DATA;

void leds_effect1_init(effect1_DATA& data)
{
  data.b=0;
  data.g=0;
  data.r=255;
  data.f_dim=0.4f;
}

void leds_effect1_draw(effect1_DATA& data)
{
  leds_clear();
  if(leds_alarm_state)
  {
    for(int i=0;i<NUM_LEDS;i++)
    {
      leds_set_color_rgb(i,data.r,data.g,data.b);
    }
  }
}

void leds_effect1_iter(effect1_DATA& data)
{
  data.r=uint32_t(data.r*data.f_dim);
  if(data.r<10)
    data.r=255;
}

void leds_alarm_task(void *pvParameters) {
  effect1_DATA effect_data;
  leds_effect1_init(effect_data);
  while (1) {
    leds_effect1_draw(effect_data);
    leds_effect1_iter(effect_data);
    leds_update();
    vTaskDelay(10);
  }
  vTaskDelete(NULL); 
}

void leds_alarm_set(bool v){
  leds_alarm_state=v;
}