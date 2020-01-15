#include "app.h"
#include "oledlogo.h"

#define TAG "SSD1306"


/// OLED - buffered output
NanoEngine<TILE_128x64_MONO> oled_engine;

void oled_task_init(void)
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    ssd1306_128x64_i2c_initEx(PIN_IIC_SCL, PIN_IIC_SDA, 0x3C);
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    ESP_LOGD(__FUNCTION__, "Successfully started...");
}

void oled_draw_logo(void) 
{
    ESP_LOGD(__FUNCTION__, "Starting...");
    ssd1306_clearScreen();
    uint32_t s=16;
    ssd1306_printFixed(s,  0, "NNC Badge  2019", STYLE_BOLD);
    ssd1306_printFixed(s, 16, "PacketMonitor &", STYLE_NORMAL);
    ssd1306_printFixed(s, 24, "DeAuth detector", STYLE_NORMAL);
    ssd1306_printFixed(s, 32, "   TechMaker©  ", STYLE_BOLD);
    ssd1306_printFixed(s, 40, "       &       ", STYLE_ITALIC);
    ssd1306_printFixed(s, 48, "      VVS      ", STYLE_NORMAL);
    Delay(2000);

    //draw
    ssd1306_clearScreen();
    ssd1306_drawBuffer(32, 0, 64, 64, nnclogo);
    Delay(500);
    ssd1306_clearScreen();
    ssd1306_drawBuffer(32, 0, 64, 64, tm_logo);
    Delay(500);
    ssd1306_clearScreen();
    ssd1306_drawBuffer(0, 0, 128, 64, vvs_logo);
    Delay(500);
}

int oled_width(void){
    return ssd1306_displayWidth();
}

int oled_height(void){
    return ssd1306_displayHeight();
}

/////////////////////////////////////////////////////////////////////////////////////////
// BUFFERED
void oled_clear(bool fForceRefresh)
{
    //ssd1306_clearScreen();
    oled_engine.canvas.clear();
    if(fForceRefresh)
        oled_engine.display();    
}

void oled_print(uint8_t x, uint8_t y, const char *text, EFontStyle style, bool fForceRefresh)
{
    //ssd1306_printFixed(x,y,ch,style);
    oled_engine.canvas.printFixed(x,y,text,style);
    if(fForceRefresh)
        oled_engine.display();
}

void oled_print(uint8_t x, uint8_t y, int value, EFontStyle style, bool fForceRefresh)
{
    char buf[16];
    itoa(value,buf,10);
    oled_engine.canvas.printFixed(x, y, buf, style);    
    if(fForceRefresh)
        oled_engine.display();
}

void oled_printf(uint8_t x, uint8_t y, EFontStyle style, const char* format, ... ) {
    char buf[32];
    memset(buf,0,sizeof(buf));
    va_list args;
    va_start( args, format );
    vsnprintf(buf, sizeof(buf)-1, format, args );
    oled_print(x,y,buf,style);
    va_end( args );
}

void oled_printf_refresh(uint8_t x, uint8_t y, EFontStyle style, const char* format, ... ) {
    char buf[32];
    memset(buf,0,sizeof(buf));
    va_list args;
    va_start( args, format );
    vsnprintf(buf, sizeof(buf)-1, format, args );
    oled_print(x,y,buf,style);
    va_end( args );
    oled_engine.display();
}

void oled_engine_init()
{
    ssd1306_clearScreen();
    oled_engine.begin();
    oled_engine.setFrameRate( 1 );
    oled_engine.canvas.setMode(CANVAS_TEXT_WRAP);
    oled_engine.refresh();
}

void oled_drawHLine(lcdint_t x1, lcdint_t y1, lcdint_t x2)
{
    oled_engine.canvas.drawHLine(x1, y1, x2);
}

void oled_drawVLine(lcdint_t x1, lcdint_t y1, lcdint_t y2)
{
    oled_engine.canvas.drawVLine(x1, y1, y2);
}

void oled_fillRect(lcdint_t x1, lcdint_t y1, lcdint_t x2, lcdint_t y2)
{
    oled_engine.canvas.fillRect(x1, y1, x2, y2);
}

void oled_refresh(void){
    oled_engine.display();
}