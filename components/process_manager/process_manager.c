#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "process_manager.h"
#include "http_handler.h"
#include "button.h"
#include "fonts.h"
#include "wifi.h"
#include "ntp_time.h"
#include "i2c_display.h"
#include "antminer.h"

#define LED_PIN 2

static const char *TAG = "PROCESS_MANAGER";

void gpio_init(void){
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_PIN, 0);
}

void wifi_process(void){
	wifi_init();
	wifi_scan_init();	
}

void display_process(void){
	i2c_procedure();

	if(oled_init() != ESP_OK){
		ESP_LOGE(TAG, "Failed to initialize the display");
		return;
	}
}


void wifi_ntp_start(void){
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
}

void main_loop(void){

	wifi_connect();
	while(!wifi_is_connected()){
		ESP_LOGI(TAG, "Connecting to Wi-Fi...");
		gpio_set_level(LED_PIN, 0);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
	gpio_set_level(LED_PIN, 1);
	wifi_ntp_start();
//	mqtt_antminer_start();
	u32 display_timer = 0;
	u32 antminer_timer = 0;
	char time[9];

	while(1){
		
		if(++display_timer >= 10){
			oled_draw_time(get_current_time_str());
			weather_response_t w = get_current_weather();
			c_print(YEL, "Weather temp: %d", w.temp);
			display_timer = 0;
		}
		if(++antminer_timer >= 50) {
			get_miner_info();
//			oled_draw_miner_info();
			antminer_timer = 0;
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}


void project_cleanup(void){
	wifi_cleanup();
	oled_clear();
	i2c_cleanup();
}
