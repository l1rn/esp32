//#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "process_manager.h"
//#include "http_handler.h"
//#include "button.h"
//#include "fonts.h"
#include "wifi.h"
#include "esp_http_server.h"
#include "temperature_sensor.h"
#include "http_server.h"
//#include "ntp_time.h"
//#include "i2c_display.h"
//#include "temp.h"
//#include "antminer.h"

//#define LED_PIN 2
//
//#define CORE0 0
//#define CORE1 ((CONFIG_FREERTOS_NUMBER_OF_CORES > 1) ? 1 : tskNO_AFFINITY)
//
//#define ANTMINER_IP_1 CONFIG_ANTMINER_IP_1
//#define ANTMINER_IP_2 CONFIG_ANTMINER_IP_2
//
//#define SCAN_WIFI 0
//#define TASK_WIFI_DONE_BIT (1 << 0)
//
//
//EventGroupHandle_t xEventGroup;
//
//static const char *TAG = "PROCESS_MANAGER";
//char *bitcoin_price = "0";
//
//void weather_task(void *pvParameters){
//	for(;;){
//	
//	}
//}
//
//void bitcoin_price_task(void *pvParameters){
//	for(;;){
//		vTaskDelay(20000 / portTICK_PERIOD_MS);
//		if(*bitcoin_price == '0'){
//			xEventGroupWaitBits(xEventGroup, TASK_WIFI_DONE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
//			bitcoin_price = get_bitcoin_price();
//			ESP_LOGI(TAG, "%s", bitcoin_price);
//			
//			xEventGroupClearBits(xEventGroup, TASK_WIFI_DONE_BIT);
//		}
//		else {
//			bitcoin_price = get_bitcoin_price();
//			ESP_LOGI(TAG, "%s", bitcoin_price);
//		}
//	}
//}
//
//void miner_task(void *pvParameters){
//	for(;;){
//		miner_response_t m = get_miner_info(ANTMINER_IP_1);
//		oled_draw_miner_info(m, bitcoin_price);
//		vTaskDelay(30000 / portTICK_PERIOD_MS);
//	}
//}
//
//void ntp_setup(void){
//	init_ntp_time();
//	int timeout = 0;
//	while(!is_time_synced() && timeout < 10){
//		printf("Waiting for sync... %d\n", timeout);
//		vTaskDelay(1000 / portTICK_PERIOD_MS);
//		timeout++;
//	}
//
//	if(is_time_synced()){
//		printf("Time: %s\n", get_current_time_str());
//	}
//}
//
//void wifi_setup_task(void *pvParameters){
//	wifi_init_sta();
////	temp_init();
//
//	if(!wifi_is_connected())
//		vTaskDelay(500 / portTICK_PERIOD_MS);
//
//	gpio_set_level(LED_PIN, 1);
//	xEventGroupSetBits(xEventGroup, TASK_WIFI_DONE_BIT);
////	ntp_setup();
//
////	xTaskCreate(miner_task, "miner", 8192, NULL, 5, NULL);
////	xTaskCreate(bitcoin_price_task, "btc", 8192, NULL, 5, NULL);
////	xTaskCreate(temp_task, "temp", 4096, NULL, 5, NULL);
//	//	xTaskCreate(weather_task, "weather", 4096, NULL, 5, NULL);
//	//for(;;){
//	//	if(is_time_synced())
//	//		oled_draw_time(get_current_time_str());
//
//	//	vTaskDelay(1000 / portTICK_PERIOD_MS);
//	//}
//
//	vTaskDelete(NULL);	
//}
//
//
//void gpio_init(void){
//	gpio_reset_pin(LED_PIN);
//	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
//	gpio_set_level(LED_PIN, 0);
//}
//
//void wifi_process(void){
//	wifi_init_sta();
//#ifdef SCAN_WIFI
//	wifi_scan_init();	
//#endif
//}
//
//void display_process(void){
//
//	gpio_init();
//	i2c_procedure();
//
//	if(oled_init() != ESP_OK){
//		ESP_LOGE(TAG, "Failed to initialize the display");
//		return;
//	}
//
//
//}

void main_loop(void){
	wifi_init_sta();
	init_temperature_config();
	httpd_handle_t server = start_server();
	while(1){
		check_rssi();
		print_temperature();
		vTaskDelay(1000);
	}
}


void project_cleanup(void){
	wifi_cleanup();
//	oled_clear();
//	i2c_cleanup();
//	temp_cleanup();
}
