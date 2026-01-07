#include "wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define SSID CONFIG_WIFI_SSID
#define PASS CONFIG_WIFI_PASSWORD
static const char *TAG = "WIFI";

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
	ESP_ERROR_CHECK(esp_wifi_connect());

	ESP_LOGI(TAG, "wifi inited");
	return ESP_OK;
}

bool wifi_is_connected(void){
	wifi_ap_record_t ap;
	return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
}
