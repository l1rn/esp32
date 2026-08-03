#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_now.h"
#include "process_manager.h"
#include "wifi_component.h"
#include "esp_http_server.h"
#include "temperature_sensor.h"
#include "http_server.h"
#include "ota.h"
#include "espnow_ota_server.h"

static const char *TAG = "PROCESS_MANAGER";

extern const unsigned char firmware_bin_start[] asm("_binary_blink_bin_start");
extern const unsigned char firmware_bin_end[] asm("_binary_blink_bin_end");

static uint8_t receiver_mac[6] = {0xB4, 0xBF, 0xE9, 0x0D, 0x5C, 0x9C};

void main_loop(void){
//	start_softap_sta();
	start_wifi_sta();
//	espnow_ota_server_init(receiver_mac);

	size_t bin_size = firmware_bin_end - firmware_bin_start;
	espnow_ota_server_start_transfer(firmware_bin_start, bin_size);

	init_temperature_config();
	httpd_handle_t server = start_server();
	esp_err_t ret = register_ota_uri(server);	

	printf("heeloo");
	if (ret != ESP_OK){
		ESP_LOGI(TAG, "Failed to run ota");
	}
}

