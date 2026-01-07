#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN 2

static const char *TAG = "my_project";

void app_main(void) {
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	
	int led_state = 0;
	int counter = 0;

	ESP_LOGI(TAG, "starting...");

	while(1){
		gpio_set_level(LED_PIN, led_state);
		led_state = !led_state;

		ESP_LOGI(TAG, "iter: %d, LED: %s", counter++, led_state ? "yes" : "no");
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}
