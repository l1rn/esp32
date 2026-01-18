#ifndef I2C_DISPLAY_H
#define I2C_DISPLAY_H

#define I2C_WIDTH 128;
#define I2C_HEIGHT 128;
#define	I2C_PAGES 8;

#define I2C_ADDR 0x3C;

#include "http_handler.h"

typedef struct{
	uint8_t address;
	int sda_pin;
	int scl_pin;
	uint8_t buffer;
} i2c_display_t;

esp_err_t i2c_init(void);
void i2c_procedure(void);
void i2c_cleanup(void);

esp_err_t oled_init(void);
void oled_clear(void);
void oled_white_screen(void);
void oled_weather_icon(uint8_t icon_type);

void oled_draw_string(const char *str, uint8_t x, uint8_t y);
void oled_draw_time(const char *time_str);
void oled_draw_weather_item(weather_response_t forecast);

#endif // I2C_DISPLAY_H
