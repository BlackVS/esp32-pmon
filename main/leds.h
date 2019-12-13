#pragma once

#include <driver/gpio.h>
#include <math.h>

#define LED_GREEN 0xFF0000
#define LED_RED 0x00FF00
#define LED_BLUE 0x0000FF

#define LED_RGB(r,g,b)  (uint32_t)(((uint32_t)g<<16)|((uint32_t)r<<8)|b);
#define B(c)  ((c)&0xff)
#define R(c)  (((c)>>8)&0xff)
#define G(c)  (((c)>>16)&0xff)

//to use with xTaskCreate
void leds_task(void *pvParameters);
void leds_alarm_task(void *pvParameters);

///
void leds_alarm_set(bool v);

// should be called only once
void leds_task_init(void);



//
void leds_set_color_raw(int index, uint32_t color);
void leds_set_color_rgb(int index, uint32_t r, uint32_t g, uint32_t b);
void leds_add_color_rgb(int index, uint32_t ra, uint32_t ga, uint32_t ba);
uint32_t leds_get_color_raw(int index);
void leds_update(void);
void leds_clear(void);
void leds_set_brightness(uint32_t br);