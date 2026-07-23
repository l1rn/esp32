#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "regex.h"
#include "esp_err.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"
#include "http_handler.h"

#define MAX_RETRY CONFIG_WIFI_MAX_RETRY 

#ifdef CONFIG_NETWORK_USE_SCAN_CHANNEL_BITMAP
#define USE_CHANNEL_BITMAP 1
#define CHANNEL_LIST_SIZE 3
static u8 channel_list[CHANNEL_LIST_SIZE] = {1,6,11};
#endif

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define SSID CONFIG_WIFI_SSID_1
#define PASS CONFIG_WIFI_PASSWORD_1

#define MAX_SCAN_AP 10

wifi_ap_t wifi_aps[5];

static const char *TAG = "WIFI";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {
	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
		esp_wifi_connect();
	} else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
		if(s_retry_num < MAX_RETRY){
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP: %d", s_retry_num);
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG, "connect to the AP fail");
	} else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "got ip:"IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

static void get_wifi_strength(int8_t rssi, char *result){
	if(rssi <= -50)	strcpy(result, "Excellent (█████)"); 
	if(rssi <= -60)	strcpy(result, "Good (████ )");
	if(rssi <= -75) strcpy(result, "Fair (███  )");
	if(rssi <= -85) strcpy(result, "Poor (██   )");
	if(rssi <= -90) strcpy(result, "Very poor (█    )");
	else result = "Unknown";
}

#ifdef USE_CHANNEL_BITMAP
static void array_2_channel_bitmap(const uint8_t channel_list[], const uint8_t channel_list_size, wifi_scan_config_t *scan_config) {
	for(uint8_t i = 0; i < channel_list_size; i++) {
		uint8_t channel = channel_list[i];
		scan_config->channel_bitmap.ghz_2_channels |= (1 << channel);
	}
}
#endif // USE_CHANNEL_BITMAP

void wifi_configure(void) {
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(TAG, "Wifi is configured\n");
}

void wifi_setup_aps(void){
	parse_wifi_json_file("wifi_aps", wifi_aps);
}

esp_err_t wifi_scan_ap(const char *specific_ssid, uint16_t max_results, 
	wifi_ap_record_t *ap_records, uint16_t *ap_count){
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));	

	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_scan_config_t scan_config = {
		.ssid = (uint8_t*)specific_ssid,
		.scan_type = WIFI_SCAN_TYPE_ACTIVE,
		.scan_time = {
			.active = {
				.min = 100,
				.max = 300,
			}
		}
	};

	ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(ap_count));

	uint16_t number_to_copy = (*ap_count > max_results) ? max_results : *ap_count; 

	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number_to_copy, ap_records));
	
	*ap_count = number_to_copy;

	esp_wifi_stop();
	esp_wifi_deinit();

	ESP_LOGI(TAG, "Scan completed. Found %d APs", *ap_count);
	return ESP_OK;
}

// TODO: make a value that will collect info & add checker for security method
void wifi_scan_array(void){
#ifdef NETWORK_USE_SCAN_LOOP
#else 
	uint16_t ap_count = 0;
	static wifi_ap_record_t ap_records[DEFAULT_SCAN_LIST_SIZE]; 
	for(int i = 0; i < 3; i++){
		memset(ap_records, 0, sizeof(ap_records));
		ESP_LOGI(TAG, "scanning for AP %d: %s", i + 1, wifi_aps[i].ssid);

		wifi_scan_ap(wifi_aps[i].ssid, 5, ap_records, &ap_count);
		ESP_LOGI(TAG, "AP_RECORD: %s, rssi: %d", ap_records[i].ssid, ap_records[i].rssi);

		if(ap_records[i].rssi == 0 || ap_records[i].ssid[0] == '\0'){
			wifi_aps[i].available = false;
		} 
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
#endif // NETWORK_USE_SCAN_LOOP
}

void wifi_init_sta(){
        s_wifi_event_group = xEventGroupCreate();

	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(TAG, "Wifi is configured\n");

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
							ESP_EVENT_ANY_ID,
							&event_handler,
							NULL,
							&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
							IP_EVENT_STA_GOT_IP,
							&event_handler,
							NULL,
							&instance_got_ip));


	wifi_config_t wifi = { .sta = { .ssid = SSID, .password = PASS  }};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_sta finished");
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
			WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
			pdFALSE,
			pdFALSE,
			portMAX_DELAY);

	if(bits & WIFI_CONNECTED_BIT){
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
				SSID, PASS);
	} else if(bits & WIFI_FAIL_BIT){
		ESP_LOGI(TAG, "failed to connect to ap SSID:%s password:%s",
				SSID, PASS);
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}
}

wifi_config_t wifi_get_priority(void){
	wifi_aps[0] = (wifi_ap_t){ .ssid = SSID, .password = PASS };

	wifi_ap_t w_ap = {0};
	for(int i = 0; i < 3; i++){
			w_ap = wifi_aps[0];
	}	
	ESP_LOGI(TAG, "w_ap: %s, priority: %d", w_ap.ssid, w_ap.priority);

	static wifi_config_t config = {0};
	strncpy((char*)config.sta.ssid, w_ap.ssid, sizeof(config.sta.ssid) - 1);
	config.sta.ssid[sizeof(config.sta.ssid) - 1] = '\0';

	strncpy((char*)config.sta.password, w_ap.password, sizeof(config.sta.password) - 1);
	config.sta.password[sizeof(config.sta.password) - 1] = '\0';	

	return config;
}

void wifi_cleanup(void){
	esp_wifi_disconnect();
	esp_wifi_stop();
	esp_wifi_deinit();
}

bool wifi_is_connected(void){
	wifi_ap_record_t ap;
	return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
}
