#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "process_manager.h"
#include "wifi.h"
#include "temperature_sensor.h"
#include "http_server.h"

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
}
