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

#define CORE0 0
#define CORE1 ((CONFIG_FREERTOS_NUMBER_OF_CORES > 1) ? 1 : tskNO_AFFINITY)

#define ANTMINER_IP_1 CONFIG_ANTMINER_IP_1
#define ANTMINER_IP_2 CONFIg_ANTMINER_IP_2

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
	wifi_init_sta();
	gpio_set_level(LED_PIN, 1);
	wifi_ntp_start();
	u32 antminer_timer = 0;
	oled_clear();
	while(1){
		oled_draw_time(get_current_time_str());
		if(++antminer_timer >= 10) {
//			get_miner_info();

//			oled_draw_weather();
			weather_response_t w = get_weather_current();
			miner_response_t m = {0};
			get_miner_info(ANTMINER_IP_1, &m);
			ESP_LOGI(TAG, "rate: %d", m.rate_avg);
			oled_draw_miner_info(m, "1111");
			antminer_timer = 0;
		}


		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void project_cleanup(void){
	wifi_cleanup();
	oled_clear();
	i2c_cleanup();
}
