#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#define SCL_IO	22
#define SDA_IO	21
#define OLED_ADDR 0x3C

static const char *TAG = "OLED DISPLAY"; 

static uint8_t display_buffer[1024];

int check_gpio_pins(void) {
	ESP_LOGI(TAG, "d21 & d22 (sda & scl) checking...");
	
	gpio_set_direction(SCL_IO, GPIO_MODE_INPUT);
	gpio_set_direction(SDA_IO, GPIO_MODE_INPUT);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	int sda_level = gpio_get_level(SDA_IO);
	int scl_level = gpio_get_level(SCL_IO);

	ESP_LOGI(TAG, "sda (gpi0%d): %s", SDA_IO, sda_level ? "HIGH" : "LOW");
	ESP_LOGI(TAG, "scl (gpi0%d): %s", SCL_IO, scl_level ? "HIGH" : "LOW");

	if(sda_level == 0 || scl_level == 0){
		ESP_LOGW(TAG, "One of those wires are not connected! Can't proceed further");
		return 0;
	}
	ESP_LOGI(TAG, "sda & scl are connected!");
	return 1;
}

esp_err_t i2c_init(void) {
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = SDA_IO,
		.scl_io_num = SCL_IO,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 100000
	};
	
	esp_err_t ret = i2c_param_config(0, &conf);
	if(ret != ESP_OK){
		ESP_LOGE(TAG, "Failed to make I2C bus: %s", esp_err_to_name(ret));
		return ret;
	}

	ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	if(ret != ESP_OK){
		ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
		return ret;
	}

	ESP_LOGI(TAG, "I2C initialized.");
	return ESP_OK;
}

char *i2c_scan(){
	ESP_LOGI(TAG, "i2c scanning");
	static char result[64] = "Not Found";
	for(uint8_t addr = 1; addr < 127; addr++){
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
		i2c_master_stop(cmd);

		esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50);
		i2c_cmd_link_delete(cmd);
		if(ret == ESP_OK){
			printf("Found: 0x%02X\n", addr);
			if(addr == 0x3C || addr == 0x3D) {
				printf("- (OLED display (sdd1306))\n");
				snprintf(result, sizeof(result), "the device has been found: 0x%02X", addr);
				return result;
			}
		} else {
			printf("\033[33mFailed on the addr: 0x%02X\n\033[0m", addr);
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	return result;
}

void i2c_cleanup(void){
	i2c_driver_delete(I2C_NUM_0);
}

void i2c_procedure(void){
	if(!check_gpio_pins()) {
		ESP_LOGE(TAG, "Failed to connect display");
		return;
	}
	if(i2c_init() != ESP_OK){
		ESP_LOGE(TAG, "Failed to init");
		return;
	} 	

	char *scan_result = i2c_scan(); 
	ESP_LOGI(TAG, "Device status: %s", scan_result);	

}

static void oled_cmd(uint8_t cmd_t){
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, OLED_ADDR << 1 | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, 0x00, true);
	i2c_master_write_byte(cmd, cmd_t, true);
	i2c_master_stop(cmd);
	
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 100 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}

static void oled_data(uint8_t data) {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, OLED_ADDR << 1 | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, 0x40, true);
	i2c_master_write_byte(cmd, data, true);
	i2c_master_stop(cmd);

	i2c_master_cmd_begin(I2C_NUM_0, cmd, 100 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}

esp_err_t oled_init(void){
	ESP_LOGI(TAG, "Initializing OLED display");
	vTaskDelay(100 / portTICK_PERIOD_MS);
	oled_cmd(0xAE);

	oled_cmd(0xD5);
	oled_cmd(0x80);

	oled_cmd(0xA8);
	oled_cmd(0x3F);

	oled_cmd(0xD3);
	oled_cmd(0x00);

	oled_cmd(0x40);
	
	oled_cmd(0x8D);
	oled_cmd(0x14);

	oled_cmd(0x20);
	oled_cmd(0x00);

	oled_cmd(0xA1);
	oled_cmd(0xC8);

	oled_cmd(0xDA);
	oled_cmd(0x12);
	
	oled_cmd(0x81);
	oled_cmd(0xCF);

	oled_cmd(0xD9);
	oled_cmd(0xF1);

	oled_cmd(0xDB);
	oled_cmd(0x40);

	oled_cmd(0xA4);
	oled_cmd(0xA6);

	oled_cmd(0xAF);

	ESP_LOGI(TAG, "OLED initialized");
	return ESP_OK;
}

void oled_clear(void){
	ESP_LOGI(TAG, "Clearing display...");

	oled_cmd(0x20);
	oled_cmd(0x00);

	oled_cmd(0x21);
	oled_cmd(0x00);
	oled_cmd(0x7F);

	oled_cmd(0x22);
	oled_cmd(0x00);
	oled_cmd(0x07);
	
	for(uint16_t i = 0; i < 128 * 8; i++){
		oled_data(0x00);
	}

	ESP_LOGI(TAG, "Display cleared");
}

void oled_white_screen(void){
	ESP_LOGI(TAG, "Making white screen...");

	uint8_t white_data[128];

	for(int i = 0; i < 128; i++){
		white_data[i] = 0xFF;
	}

	for(uint8_t page = 0; page < 8; page++){
		oled_cmd(0xB0 + page);
		oled_cmd(0x00);
		oled_cmd(0x10);

		for(int col = 0; col < 128; col++){
			oled_data(0xFF);
		}
	}

	ESP_LOGI(TAG, "White screen done!");
}

void oled_weather_icon(uint8_t icon_type) {
	static const uint8_t icon_sunny[8] = {
		0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	const uint8_t *icon_data;
	
	switch(icon_type){
		case 0: 
			icon_data = icon_sunny; 
			break;
		default: 
			icon_data = icon_sunny;
			break;
	}

	for(uint8_t page = 0; page < 2; page++){
		oled_cmd(0xB0 + page);
		
		oled_cmd(0x00 + (112 & 0x0F));
		oled_cmd(0x10 + ((112 >> 4) & 0x0F));

		for(uint8_t col = 0; col < 6; col++){
			oled_data(icon_data[page * 4 + col]);
		}
	}
}
