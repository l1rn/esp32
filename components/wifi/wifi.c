#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "reges.h"

#include "colorize.h"
#include "wifi.h"

#define SSID CONFIG_WIFI_SSID
#define PASS CONFIG_WIFI_PASSWORD

#define DEFAULT_SCAN_LIST_SIZE 32

static const char *TAG = "WIFI";

#ifdef USE_CHANNEL_BITMAP
static void array_2_channel_bitmap(const uint8_t channel_list[], const uint8_t channel_list_size, wifi_scan_config_t *scan_config) {
	for(uint8_t i = 0; i < channel_list_size; i++) {
		uint8_t channel = channel_list[i];
		scan_config->channel_bitmap.ghz_2_channels |= (1 << channel);
	}
}
#endif // USE_CHANNEL_BITMAP

esp_err_t wifi_init(void) {
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	esp_netif_init();
	esp_event_loop_create_default();
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	
	wifi_config_t wifi = { .sta = { .ssid = SSID, .password = PASS } };

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
	ESP_ERROR_CHECK(esp_wifi_start());

	c_printf(LGBL_WHT, "\nWifi initialized\n");

	return ESP_OK;
}

void wifi_start(void){
}

bool wifi_is_connected(void){
	wifi_ap_record_t ap;
	return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
}


