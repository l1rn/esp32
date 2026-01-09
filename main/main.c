#include <stdio.h>
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "antminer.h"

#define LED_PIN 2

static const char *TAG = "MAIN";

void app_main(void) {
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	
	wifi_init();
	while(!wifi_is_connected()){
		ESP_LOGI(TAG, "Waiting for Wifi...");
		gpio_set_level(LED_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	ESP_LOGI(TAG, "Wifi connected!");
	gpio_set_level(LED_PIN, 1);
	
	antminer_init("192.168.1.129", "root", "root");
	antminer_data_t data;
	while(1) {
		esp_err_t err = antminer_get_data(&data);
		if(err == ESP_OK) {
			ESP_LOGI(TAG, "Antminer data received!");
		} else {
			ESP_LOGE(TAG, "Antminer request failed: %s", esp_err_to_name(err));
		}
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}
