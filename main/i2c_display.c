#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

#define I2C_MASTER_SCL_IO	22
#define I2C_MASTER_SDA_IO	21
#define I2C_MASTER_NUM		I2C_NUM_0
#define I2C_MASTER_FREQ_HZ	100000
#define OLED_ADDR		0x3C

static const char *TAG = "OLED DISPLAY"; 

void i2c_master_init(i2c_master_bus_handle_t *bus) {
	i2c_master_bus_config_t cfg = {
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.i2c_port = 0,
		.scl_io_num = I2C_MASTER_SCL_IO,
		.sda_io_num = I2C_MASTER_SDA_IO,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};
	i2c_new_master_bus(&cfg, bus);

	ESP_LOGI(TAG, "i2c initialized.");
}

uint8_t check_addr(i2c_master_bus_handle_t bus, uint8_t addr){
	i2c_device_config_t dev_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = addr,
		.scl_speed_hz = 100000,
	};

	i2c_master_dev_handle_t dev;
	if(i2c_master_bus_add_device(bus, &dev_cfg, &dev) != ESP_OK){
		return 0;
	}

	uint8_t dummy = 0;
	esp_err_t res = i2c_master_transmit(dev, &dummy, 0, 100);
	
	i2c_master_bus_rm_device(dev);
	return (res == ESP_OK) ? 1 : 0;
}

void scan_i2c(i2c_master_bus_handle_t bus){
	ESP_LOGI(TAG, "i2c scanning");
	uint8_t found = 0;
	for(uint8_t addr = 1; addr < 127; addr++){
		if(check_addr(bus, addr)){
			ESP_LOGI(TAG, "FOUND - 0x%02X", addr);
			found++;
		}
		vTaskDelay(1);
	}
	
}
