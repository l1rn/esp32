#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "process_manager.h"
#include "wifi.h"
#include "ntp_time.h"
#include "i2c_display.h"

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
	u8
	while(1){

	}
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
