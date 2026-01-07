#include <stdio.h>
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN 2

static const char *TAG = "led";

void app_main(void) {
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	
	wifi_init();

	while(1) {
		if(wifi_is_connected()){
			ESP_LOGI(TAG, "Wifi Connected");
			gpio_set_level(LED_PIN, 1);
		} else{
			ESP_LOGI(TAG, "WiFi Disconnected");
			gpio_set_level(LED_PIN, 0);
		}
		vTaskDelay(3000 / portTICK_PERIOD_MS);
	}
}
