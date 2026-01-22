#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "regex.h"
#include "esp_err.h"

#include "colorize.h"
#include "wifi.h"

#define SSID CONFIG_WIFI_SSID
#define PASS CONFIG_WIFI_PASSWORD

#ifdef CONFIG_NETWORK_USE_SCAN_CHANNEL_BITMAP
#define USE_CHANNEL_BITMAP 1
#define CHANNEL_LIST_SIZE 3
static u8 channel_list[CHANNEL_LIST_SIZE] = {1,6,11};
#endif

static const char *TAG = "WIFI";

static void get_wifi_strength(int8_t rssi, char *result){
	if(rssi <= -50){
		strcpy(result, "Excellent (█████)");
	} 
	if(rssi <= -60) {
		strcpy(result, "Good (████ )");
	} 
	if(rssi <= -75) {
		strcpy(result, "Fair (███  )");
	} 
	if(rssi <= -85){
		strcpy(result, "Poor (██   )");
	}
	if(rssi <= -90){
		strcpy(result, "Very poor (█    )");
	}
	else {
		result = "Unknown";
	}
}

#ifdef USE_CHANNEL_BITMAP
static void array_2_channel_bitmap(const uint8_t channel_list[], const uint8_t channel_list_size, wifi_scan_config_t *scan_config) {
	for(uint8_t i = 0; i < channel_list_size; i++) {
		uint8_t channel = channel_list[i];
		scan_config->channel_bitmap.ghz_2_channels |= (1 << channel);
	}
}
#endif // USE_CHANNEL_BITMAP

void wifi_init(void) {
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	(void)sta_netif;

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	c_print(LGBL_WHT, "\nWifi initialized\n");
}

void wifi_scan_single_time(u16 ap_count, u16 number, wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE]){
#ifdef USE_CHANNEL_BITMAP
	wifi_scan_config_t *scan_config = (wifi_scan_config_t *)calloc(1, sizeof(wifi_scan_config_t));
	if(!scan_config){
		ESP_LOGE(TAG, "Memory Allocation for scan config failed!");
		return;
	}
	array_2_channel_bitmap(channel_list, CHANNEL_LIST_SIZE, scan_config);
	esp_wifi_scan_start(scan_config, true);
	free(scan_config);
#else 
	esp_wifi_scan_start(NULL, true);
#endif // USE_CHANNEL_BITMAP
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
	ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", 
			ap_count, number);

	printf("\n");
	char res[32];
	c_print(CYN, "+===============================================+\n");
	c_print(CYN, "| SSID       | RSSI                   | CHANNEL |\n");
	c_print(CYN, "=================================================\n");
	for(int i = 0; i < number; i++){
		get_wifi_strength(ap_info[i].rssi, res);
		c_print(CYN, "| %s | (%d)%s     | 0     | %d    |\n", 
				ap_info[i].ssid, ap_info[i].rssi, res, ap_info[i].primary);
	}
	c_print(CYN, "+===============================================+\n");
}

void wifi_scan_init(void){
	u16 number = DEFAULT_SCAN_LIST_SIZE;
	wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
	u16 ap_count = 0;
	memset(ap_info, 0, sizeof(ap_info));

	wifi_scan_single_time(ap_count, number, ap_info);
}

void wifi_connect(void){
	ESP_ERROR_CHECK(esp_wifi_stop());
	wifi_config_t wifi = { .sta = { .ssid = SSID, .password = PASS } };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());
	c_print(LGBL_WHT, "\nWifi connected\n");
}

bool wifi_is_connected(void){
	wifi_ap_record_t ap;
	return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
}


