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

static const char *TAG = "WIFI";

static int s_retry_num = 0;

wifi_ap_t wifi_aps[] = {
#ifdef CONFIG_WIFI_AP_1_ENABLE
	{
		.ssid = CONFIG_WIFI_SSID_1,
		.password = CONFIG_WIFI_PASSWORD_1,
	#if CONFIG_WIFI_PRIORITIRIZE == 1
		.prioritirize = true,
	#else
		.prioritirize = false,
	#endif
	},
#endif
#ifdef CONFIG_WIFI_AP_2_ENABLE 
	{
		.ssid = CONFIG_WIFI_SSID_2,
		.password = CONFIG_WIFI_PASSWORD_2,
	#if CONFIG_WIFI_PRIORITIRIZE == 2
		.prioritirize = true,
	#else
		.prioritirize = false,
	#endif
	},
#endif
#ifdef CONFIG_WIFI_AP_3_ENABLE
	{
		.ssid = CONFIG_WIFI_SSID_3,
		.password = CONFIG_WIFI_PASSWORD_3
	#if CONFIG_WIFI_PRIORITIRIZE == 3
		.prioritirize = true,
	#else
		.prioritirize = false,
	#endif
	},
#endif
};

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

void wifi_init(void) {
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	(void)sta_netif;

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "Wifi initialized\n");
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
	c_print(CYN, "+====================================================+\n");
	c_print(CYN, "| SSID             | RSSI                  | CHANNEL |\n");
	c_print(CYN, "=====================================================\n");
	for(int i = 0; i < number; i++){
		get_wifi_strength(ap_info[i].rssi, res);
		c_print(CYN, "| %s  (%d)%s  %d     |\n", 
				ap_info[i].ssid, ap_info[i].rssi, res, ap_info[i].primary);
	}
	c_print(CYN, "+====================================================+\n");
}

// TODO: make a value that will collect info & add checker for security method
void wifi_scan_init(void){
	u16 number = DEFAULT_SCAN_LIST_SIZE;
	wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
	u16 ap_count = 0;
	memset(ap_info, 0, sizeof(ap_info));

#ifdef NETWORK_USE_SCAN_LOOP
	while(1){
		wifi_scan_single_time(ap_count, number, ap_info);
		vTaskDelay(15000 / portTICK_PERIOD_MS);
	}
#else 
	wifi_scan_single_time(ap_count, number, ap_info);
#endif // NETWORK_USE_SCAN_LOOP
}

static bool check_connection_to_sta();

void wifi_init_sta(void){
        s_wifi_event_group = xEventGroupCreate();
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

	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&config));	

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


	wifi_config_t wifi = { .sta = { .ssid = CONFIG_WIFI_SSID_1, .password = CONFIG_WIFI_PASSWORD_1} };
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

void wifi_priority_connect(void){
	wifi_ap_t w_ap = {0};
	for(int i = 0; i < 3; i++){
		if(wifi_aps[i].prioritize){
			w_ap = wifi_aps[i];
		}
	}	

	wifi_config_t config = {0};
	strncpy((char*)config.sta.ssid, w_ap.ssid, sizeof(config.sta.ssid) - 1);
	config.sta.ssid[sizeof(config.sta.ssid) - 1] = '\0';

	strncpy((char*)config.sta.password, w_ap.password, sizeof(config.sta.password) - 1);
	config.sta.password[sizeof(config.sta.password) - 1] = '\0';

	
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
