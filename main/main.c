#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/i2c_master.h"

#include "antminer.h"
#include "http_handler.h"
#include "wifi.h"
#include "i2c_display.h"
#include "ntp_time.h"

#define LED_PIN 2

static const char *TAG = "MAIN";

void app_main(void) {
	vTaskDelay(pdMS_TO_TICKS(1000));

	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_PIN, 0);

	wifi_init();
	while(!wifi_is_connected()){
		ESP_LOGI(TAG, "Waiting for Wifi...");
		gpio_set_level(LED_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	ESP_LOGI(TAG, "Wifi connected!");
	vTaskDelay(pdMS_TO_TICKS(2000));

	init_ntp_time();
	gpio_set_level(LED_PIN, 1);

	int timeout = 0;
	while(!is_time_synced() && timeout < 10){
		printf("Waiting for sync... %d\n", timeout);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		timeout++;
	}

	if(is_time_synced()){
		printf("Time: %s\n", get_current_time_str());
	}

	// mqtt_antminer_start();
	i2c_procedure();
	if(oled_init() != ESP_OK){
		ESP_LOGE(TAG, "Failed to initialize the display");
		return;
	}
	oled_clear();

	while(1) {
		get_weather_15hours();
		char *time = get_current_time_str();
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
	i2c_cleanup();
}
