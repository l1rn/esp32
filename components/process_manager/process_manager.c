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
#define ANTMINER_IP_2 CONFIG_ANTMINER_IP_2

#define SCAN_WIFI 0

static const char *TAG = "PROCESS_MANAGER";

void ntp_task(void *pvParameters){
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


void weather_task(void *pvParameters){
	for(;;){

	}
}

void miner_task(void *pvParameters){
	for(;;){
		miner_response_t m = get_miner_info(ANTMINER_IP_1);
		oled_draw_miner_info(m, "1111");
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}


void wifi_setup_task(void *pvParameters){
	wifi_init_sta();

	if(!wifi_is_connected())
		vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "WiFi is connected");

	gpio_set_level(LED_PIN, 1);

	xTaskCreate(ntp_task, "ntp", 4096, NULL, 5, NULL);
	xTaskCreate(miner_task, "miner", 4096, NULL, 5, NULL);
	//	xTaskCreate(weather_task, "weather", 4096, NULL, 5, NULL);
	for(;;){
		if(is_time_synced())
			oled_draw_time(get_current_time_str());
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);	
}


void supervisor_task(void *pvParameters){
}

void gpio_init(void){
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_PIN, 0);
}

void wifi_process(void){
	wifi_init_sta();
#ifdef SCAN_WIFI
	wifi_scan_init();	
#endif
}



void main_loop(void){
	gpio_init();
	i2c_procedure();

	if(oled_init() != ESP_OK){
		ESP_LOGE(TAG, "Failed to initialize the display");
		return;
	}

	wifi_init();
		
	oled_clear();
	xTaskCreatePinnedToCore(
			wifi_setup_task,
			"wifi_setup",
			4096, 
			NULL,
			1,
			NULL,
			CORE0
	);
}


void project_cleanup(void){
	wifi_cleanup();
	oled_clear();
	i2c_cleanup();
}
