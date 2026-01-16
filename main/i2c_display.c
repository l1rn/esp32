#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

#define SCL_IO	22
#define SDA_IO	21
#define OLED_ADDR 0x3C

static const char *TAG = "OLED DISPLAY"; 

int check_gpio_pins(void) {
	printf("d21 & d22 (sda & scl) checking...");
	
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
	
	esp_err_t ret = i2c_param_config(I2C_NUM_O, &conf);
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

int i2c_scan(){
	ESP_LOGI(TAG, "i2c scanning");
	uint8_t found = 0;
	for(uint8_t addr = 1; addr < 127; addr++){
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
		i2c_master_stop(cmd);

		esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50);
		i2c_cmd_link_delete(cmd);
		if(ret == ESP_OK){
			printf("Found: 0x%02X", addr);
			if(addr == 0x3C || addr = 0x3D) printf("OLED display (sdmm)");
			printf("\n");
			found++;
		} else {
			printf("Failed on the addr: 0x%02X", addr);
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	if(found == 0){
		ESP_LOGW(TAG, "Failed to search the display, nothing found");
		return found;
	}
	return found;
}

void i2c_cleanup(void){
	i2c_driver_delete(I2C_NUM_0);
}

void i2c_procedure(void){
	if(!check_gpio_pins()) return;
	if(i2c_init() != ESP_OK) return;
	i2c_scan();	

	i2c_cleanup();
}
