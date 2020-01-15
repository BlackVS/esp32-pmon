#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "ssd1306.h"
#include "nano_gfx.h"
#include "nano_engine.h"


extern const uint8_t nnclogo[];
extern const uint8_t tm_logo[];

int oled_width(void);
int oled_height(void);


// IO init - should be called only once
void oled_task_init(void);

// unbuffered output
void oled_draw_logo(void);

// buffered output
extern NanoEngine<TILE_128x64_MONO> oled_engine;
void oled_engine_init(void);
void oled_clear(bool fForceRefresh=false);
void oled_print(uint8_t x, uint8_t y, const char *text, EFontStyle style=STYLE_NORMAL, bool fForceRefresh=false);
void oled_print(uint8_t x, uint8_t y, int value, EFontStyle style=STYLE_NORMAL, bool fForceRefresh=false);
void oled_printf(uint8_t x, uint8_t y, EFontStyle style, const char* format, ... );
void oled_printf_refresh(uint8_t x, uint8_t y, EFontStyle style, const char* format, ... );
    
void oled_drawHLine(lcdint_t x1, lcdint_t y1, lcdint_t x2);
void oled_drawVLine(lcdint_t x1, lcdint_t y1, lcdint_t y2);
void oled_fillRect(lcdint_t x1, lcdint_t y1, lcdint_t x2, lcdint_t y2);
void oled_refresh(void);//to show buffered
//setTextSize

