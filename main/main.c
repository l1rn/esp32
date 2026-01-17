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
#include "weather.h"
#include "wifi.h"
#include "i2c_display.h"

#define LED_PIN 2

static const char *TAG = "MAIN";

void app_main(void) {
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

//	wifi_init();
//	while(!wifi_is_connected()){
//		ESP_LOGI(TAG, "Waiting for Wifi...");
///		gpio_set_level(LED_PIN, 0);
//		vTaskDelay(1000 / portTICK_PERIOD_MS);
//	}

//	ESP_LOGI(TAG, "Wifi connected!");
	gpio_set_level(LED_PIN, 1);

	// mqtt_antminer_start();
	i2c_procedure();
	if(oled_init() != ESP_OK){
		ESP_LOGE(TAG, "Failed to initialize the display");
		return;
	}
	oled_clear();
	oled_weather_icon(0);
	i2c_cleanup();
	while(1){
		vTaskDelay(1000);
	}
//	while(1) {
//		get_weather_current();

//		vTaskDelay(pdMS_TO_TICKS(5000));
//	}

}
