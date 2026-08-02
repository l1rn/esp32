#include "wifi_component.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "lwip/inet.h"
#include "nvs_flash.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif

// SOFT AP CONFIG
#define ESP_WIFI_AP_SSID CONFIG_ESP_WIFI_AP_SSID
#define ESP_WIFI_AP_PASSWORD CONFIG_ESP_WIFI_AP_PASSWORD
#define ESP_WIFI_AP_CHANNEL CONFIG_ESP_WIFI_AP_CHANNEL
#define ESP_MAX_STA_CONN_AP CONFIG_ESP_MAX_STA_CONN_AP

// STA CONFIG
#define ESP_STA_SSID CONFIG_ESP_STA_SSID
#define ESP_STA_PASSWORD CONFIG_ESP_STA_PASSWORD
#define ESP_STA_MAX_RETRY CONFIG_ESP_STA_MAX_RETRY
#define ESP_STA_CHANNEL CONFIG_ESP_STA_CHANNEL

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP 
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#ifndef ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK   
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define DHCPS_OFFER_DNS 0x02

static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "Wifi STA";

static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(
		void *arg, 
		esp_event_base_t event_base, 
		int32_t event_id, 
		void *event_data
) {
	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED){
		wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
		ESP_LOGI(TAG_AP, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
	} else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED){
		wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
		ESP_LOGI(TAG_AP, "Station "MACSTR" left, AID=%d, reason:%d", MAC2STR(event->mac), event->aid, event->reason);
	} else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
		esp_wifi_connect();
		ESP_LOGI(TAG_STA, "Station started!");
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
		if (s_retry_num < ESP_STA_MAX_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG_STA, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
	} else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
		ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
		ESP_LOGI(TAG_STA, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	} else if(event_base == IP_EVENT && event_id == IP_EVENT_ASSIGNED_IP_TO_CLIENT){
		const ip_event_assigned_ip_to_client_t *e = (const ip_event_assigned_ip_to_client_t *)event_data;
		ESP_LOGI(TAG_AP, "Assigned IP to client: " IPSTR ", MAC=" MACSTR ", hostname='%s'", 
		IP2STR(&e->ip), MAC2STR(e->mac), e->hostname);
	}
}

esp_netif_t *wifi_init_sta(void){
	esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();
	
	wifi_config_t wifi_sta_config = {
		.sta = {
			.ssid = ESP_STA_SSID,
			.password = ESP_STA_PASSWORD,
			.channel = ESP_STA_CHANNEL,
			.failure_retry_cnt = ESP_STA_MAX_RETRY,
			.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
 	
		},
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
	ESP_LOGI(TAG_STA, "wifi_init_sta connected!");
	return esp_netif_sta;
}

esp_netif_t *wifi_init_softap(void){
	esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

	wifi_config_t wifi_ap_config = {
		.ap = {
			.ssid = ESP_WIFI_AP_SSID,
			.ssid_len=strlen(ESP_WIFI_AP_SSID),
			.password = ESP_WIFI_AP_PASSWORD,
			.channel = ESP_WIFI_AP_CHANNEL,
			.max_connection = ESP_MAX_STA_CONN_AP,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
				.required = false,
			},
		},
	};

	if(strlen(ESP_WIFI_AP_PASSWORD) == 0){
		wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

	ESP_LOGI(TAG_AP, "wifi_init_softap connected! | SSID: %s | PASS: %s | CHANNEL: %d", ESP_WIFI_AP_SSID, ESP_WIFI_AP_PASSWORD, ESP_WIFI_AP_CHANNEL);
	return esp_netif_ap;
}

void softap_set_dns_addr(esp_netif_t *esp_netif_ap, esp_netif_t *esp_netif_sta){
	esp_netif_dns_info_t dns;
	esp_netif_get_dns_info(esp_netif_sta, ESP_NETIF_DNS_MAIN,&dns);
	uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
	ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
	ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));
}

void start_softap_sta(void){
	esp_err_t ret = nvs_flash_init();
 	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
 		ESP_ERROR_CHECK(nvs_flash_erase());
 		ret = nvs_flash_init();
 	}
	ESP_ERROR_CHECK(ret);
 	ESP_ERROR_CHECK(esp_netif_init());
 	ESP_ERROR_CHECK(esp_event_loop_create_default());
 
 	s_wifi_event_group = xEventGroupCreate();
 	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
 				ESP_EVENT_ANY_ID,
 				&wifi_event_handler, NULL, NULL));
 
 	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
 				IP_EVENT_STA_GOT_IP,
 				&wifi_event_handler, NULL, NULL));
 	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
 				IP_EVENT_ASSIGNED_IP_TO_CLIENT,
 				&wifi_event_handler, NULL, NULL));
 	
 	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
 	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
 
 	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
 	ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
 	esp_netif_t *esp_netif_ap = wifi_init_softap();
 
 	ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
 	esp_netif_t *esp_netif_sta = wifi_init_sta();
 
 	ESP_ERROR_CHECK(esp_wifi_start());
 
 	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, 
 				WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
 				pdFALSE,
 				pdFALSE,
 				portMAX_DELAY);
 
 	if(bits & WIFI_CONNECTED_BIT){
 		ESP_LOGI(TAG_STA, "connected to ap SSID:%s password:%s",
 				ESP_STA_SSID, ESP_STA_PASSWORD);
 		softap_set_dns_addr(esp_netif_ap, esp_netif_sta);

		esp_netif_set_default_netif(esp_netif_sta);

#if IP_NAPT 
		if(esp_netif_napt_enable(esp_netif_ap) == ESP_OK){
			ESP_LOGI(TAG_STA, "NAPT enabled successfully: %p", esp_netif_ap);
		} else {
			ESP_LOGE(TAG_AP, "Failed to enable NAPT");
		}
#endif
 	} else if(bits & WIFI_FAIL_BIT){
 		ESP_LOGI(TAG_STA, "failed to connect to SSID:%s password:%s", ESP_STA_SSID, ESP_STA_PASSWORD);
 	} else {
 		ESP_LOGE(TAG_STA, "UNEXPECTED EVENT");
 		return;
 	}


 }

void start_wifi_sta(){
	nvs_flash_init();

	esp_netif_init();
	esp_event_loop_create_default();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_start();
}
