#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "ssd1306.h"
#include "nano_gfx.h"


extern const uint8_t nnclogo[];
extern const uint8_t tm_logo[];

int oled_width(void);
int oled_height(void);


// should be called only once
void oled_task_init(void);
    

void oled_draw_logo(void);
