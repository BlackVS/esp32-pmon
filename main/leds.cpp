#include "app.h"

#include "ws2812_control.h"

static struct led_state LEDS;
static struct led_state LEDS_out;
static uint32_t leds_brightness=100;

int randrange(int mn, int mx){
  return (rand() % (mx - mn + 1)) + mn; 
}

void leds_task_init(void) 
{
  ws2812_control_init();
  leds_clear();
  leds_update();
}

void leds_set_brightness(uint32_t br)
{
  leds_brightness=br;
}

void leds_clear(void) 
{
  for (int i = 0; i < NUM_LEDS; i++) 
    leds_set_color_raw(i, 0);
}

void leds_set_color_raw(uint32_t index, uint32_t color) 
{
  if(index<NUM_LEDS)
    LEDS.leds[index] = color;
}

uint32_t leds_get_color_raw(uint32_t index) 
{
  if(index<NUM_LEDS)
    return LEDS.leds[index];
  return 0;
}

void leds_set_color_rgb(uint32_t index, uint32_t r, uint32_t g, uint32_t b) 
{
  if(index<NUM_LEDS)
  {
    r=MIN(255, r);
    g=MIN(255, g);
    b=MIN(255, b);
    LEDS.leds[index] = LED_RGB(r,g,b);
  }
}

void leds_add_color_rgb(uint32_t index, uint32_t ra, uint32_t ga, uint32_t ba) 
{
  if(index<NUM_LEDS)
  {
    uint32_t r, g, b, c;
    c=leds_get_color_raw(index);
    r=R(c)+ra;
    g=G(c)+ga;
    b=B(c)+ba;
    leds_set_color_rgb(index,r,g,b);
  }
}

void leds_update() 
{ 
  uint32_t bmax=leds_brightness;
  uint32_t r, g, b;
  for(int i=0; i<NUM_LEDS; i++)
  {
      uint32_t c = LEDS.leds[i];
      r=(R(c)*bmax)/255;
      g=(G(c)*bmax)/255;
      b=(B(c)*bmax)/255;
      LEDS_out.leds[i]=LED_RGB(r,g,b);
  }
  ws2812_write_leds(LEDS_out); 
}

typedef struct {
  uint32_t r1, g1, b1, pos1, dir1;
  uint32_t r2, g2, b2, pos2, dir2;
  float f_dim;
} EFFECT0_DATA;

void leds_effect0_rand(EFFECT0_DATA& data){
  data.b1=randrange(0,255);
  data.g1=randrange(0,255);
  data.r1=randrange(0,255);
  data.dir1 = rand()> (RAND_MAX/2) ? 1 : -1;
  data.b2=randrange(0,255);
  data.g2=randrange(0,255);
  data.r2=randrange(0,255);
  data.dir2 = rand()> (RAND_MAX/2) ? 1 : -1;
}

void leds_effect0_init(EFFECT0_DATA& data)
{
  leds_effect0_rand(data);
  data.pos1=0;
  data.pos2=NUM_LEDS/2;
  data.f_dim=0.4f;
}


void leds_effect0_dim(float f)
{
  for(int i=0; i<NUM_LEDS; i++){
    uint32_t c=leds_get_color_raw(i);
    uint32_t r=(uint32_t)(R(c)*f);
    uint32_t g=(uint32_t)(G(c)*f);
    uint32_t b=(uint32_t)(B(c)*f);
    leds_set_color_rgb(i,r,g,b);
  }
}

void leds_effect0_draw(EFFECT0_DATA& data)
{
  leds_clear();
  uint32_t r,g,b,c,p;
  uint32_t rc,gc,bc;
  rc = data.r1;
  gc = data.g1;
  bc = data.b1;
  p=data.pos1;
  for(int i=0;i<NUM_LEDS;i++,p=(p-data.dir1+NUM_LEDS)%NUM_LEDS)
  {
    c=leds_get_color_raw(p);
    r=R(c)+rc;
    g=G(c)+gc;
    b=B(c)+bc;
    leds_set_color_rgb(p,r,g,b);
    rc=(uint32_t)(rc*data.f_dim);
    gc=(uint32_t)(gc*data.f_dim);
    bc=(uint32_t)(bc*data.f_dim);
  }
  rc = data.r2;
  gc = data.g2;
  bc = data.b2;
  p=data.pos2;
  for(int i=0;i<NUM_LEDS;i++,p=(p-data.dir2+NUM_LEDS)%NUM_LEDS)
  {
    c=leds_get_color_raw(p);
    r=R(c)+rc;
    g=G(c)+gc;
    b=B(c)+bc;
    leds_set_color_rgb(p,r,g,b);
    rc=(uint32_t)(rc*data.f_dim);
    gc=(uint32_t)(gc*data.f_dim);
    bc=(uint32_t)(bc*data.f_dim);
  }
}

void leds_effect0_iter(EFFECT0_DATA& data)
{
  data.pos1=(data.pos1+data.dir1+NUM_LEDS)%NUM_LEDS;
  data.pos2=(data.pos2+data.dir2+NUM_LEDS)%NUM_LEDS;
}

void leds_task(void *pvParameters) {
  EFFECT0_DATA effect_data;
  leds_effect0_init(effect_data);
  float f=1.0f;
  while (1) {
    leds_effect0_draw(effect_data);
    leds_effect0_iter(effect_data);
    leds_effect0_dim(f);
    if (touchpad_get_state()==TOUCHPAD_STATE_ON)
      f=f*0.8f;
    else
      f=f*1.2f;
    if(f<0.01f) {
      f=0.01f;
      leds_effect0_rand(effect_data);
    }
    if(f>0.95f){
      f=1.0f;
    }
    leds_update();
    vTaskDelay(10);
  }
  vTaskDelete(NULL); 
}
