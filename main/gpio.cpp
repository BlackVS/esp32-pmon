#include "app.h"

void led_gpio_init() {
   static bool fInited=false;
   if(!fInited)
   {
    gpio_config_t io_conf;
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL << PIN_LED_BLUE);
    // disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
    fInited=true;
   }
}

void mpu_gpio_init()
{
	/* Init LED */
	gpio_pad_select_gpio(BLINK_GPIO);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT_OUTPUT);
    //i2c should be already inited
}

void i2c_gpio_init(void)
{
    i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.scl_io_num = MPU_I2C_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.sda_io_num = MPU_I2C_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = MPU_I2C_FREQ_HZ;
	i2c_param_config(MPU_I2C_PORT, &conf);
	i2c_driver_install(MPU_I2C_PORT, conf.mode, 0, 0, 0);
}


/*void oled_gpio_init()
{
    i2c_port_t bus_id = ; //I2C_NUM_1;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = PIN_IIC_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = PIN_IIC_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = MPU_I2C_FREQ_HZ;
    i2c_param_config  (MPU_I2C_PORT, &conf);
    i2c_driver_install(MPU_I2C_PORT, conf.mode, 0, 0, 0);
}*/

void gpio_init(void)
{
    i2c_gpio_init(); //no need to init if oled present - inited inside oled toolkit
    led_gpio_init();
    mpu_gpio_init();
    //oled_gpio_init();
}