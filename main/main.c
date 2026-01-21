#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "antminer.h"
#include "http_handler.h"
#include "wifi.h"
#include "i2c_display.h"
#include "ntp_time.h"
#include "process_manager.h"

#define LED_PIN 2

static const char *TAG = "MAIN";

void app_main(void) {
	vTaskDelay(pdMS_TO_TICKS(1000));

	i2c_procedure();
	if(oled_init() != ESP_OK){
		ESP_LOGE(TAG, "Failed to initialize the display");
		return;
	}

	// wifi
	wifi_init();
	while(!wifi_is_connected()){
		ESP_LOGI(TAG, "Waiting for Wifi...");
		gpio_set_level(LED_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	ESP_LOGI(TAG, "Wifi connected!");
	
	// delay before connection & ntp init
	vTaskDelay(pdMS_TO_TICKS(2000));
	init_ntp_time();

	int timeout = 0;
	while(!is_time_synced() && timeout < 10){
		printf("Waiting for sync... %d\n", timeout);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		timeout++;
	}

	if(is_time_synced()){
		printf("Time: %s\n", get_current_time_str());
	}

	mqtt_antminer_start();
	oled_clear();

	weather_response_t forecasts[MAX_FORECASTS];
	weather_response_t weather;
	
	uint32_t localtime_counter = 0;
	uint32_t weather_counter = 0;
	uint32_t antminer_counter = 0;
	// main cycle
	while(1) {
			
		if(localtime_counter % 2 == 0){
			if(is_time_synced()){
				char *time = get_current_time_str();
				oled_draw_time(time);
			}
		}
		if(weather_counter % 10 == 0) {
//			get_weather_15hours(forecasts, MAX_FORECASTS);
//			oled_draw_weather_item(forecasts[0]);
			get_weather_current(&weather);
			char temp[4];
			sprintf(temp, "%d", weather.temp);

			ESP_LOGI(TAG, "temp: %s", temp);
		}
		if(antminer_counter % 20 == 0){
			oled_draw_miner_info();
	//		oled_draw_digit('2', 0, 0);
	//		oled_draw_digit('1', 18, 0);
	//		oled_draw_digit('4', 0, 26);
		}
		weather_counter++;
		localtime_counter++;
		antminer_counter++;
		vTaskDelay(pdMS_TO_TICKS(500));
	}
	i2c_cleanup();
}
