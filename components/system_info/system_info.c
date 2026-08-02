#include "esp_timer.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_clk.h"
#include <stdio.h>
#include <inttypes.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include <string.h>

typedef int64_t _i64;
typedef uint32_t u32;

int get_uptime(char uptime[], int size){
	_i64 uptime_us = esp_timer_get_time();
	u32 uptime_sec = (u32)(uptime_us / 1000000);
	u32 days = uptime_sec / 86400;
	u32 hours = (uptime_sec % 86400) / 3600;
	u32 minutes = (uptime_sec % 3600) / 60;
	u32 seconds = uptime_sec % 60;

	return snprintf(uptime, size, "%lu days %02lu:%02lu:%02lu", days, hours, minutes, seconds);	
}

int get_system_info(char heap[], int size){
	float cpu_freq_mhz = (double)esp_clk_cpu_freq() / 1000000;
	u32 free_heap = esp_get_free_heap_size();
	u32 min_free_heap = esp_get_minimum_free_heap_size();
	u32 free_interval = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
	
	return snprintf(
		heap, 
		size, 
		"<li>cpu freq: %0.2f MHz</li>\n"
		"<li>free heap: %lu bytes</li>\n"
		"<li>min free heap ever used: %lu bytes</li>\n"
		"<li>free interval ram: %lu bytes</li>\n",
		cpu_freq_mhz,
		(unsigned long)free_heap,
		(unsigned long)min_free_heap,
		(unsigned long)free_interval);
}

static void get_wifi_strength(int8_t rssi, char *result){
	if(rssi <= -50)	strcpy(result, "Excellent "); 
	if(rssi <= -60)	strcpy(result, "Good ");
	if(rssi <= -75) strcpy(result, "Fair");
	if(rssi <= -85) strcpy(result, "Poor");
	if(rssi <= -90) strcpy(result, "Very poor");
	else result = "Unknown";
}

int get_wifi_bandwidth(wifi_bandwidth_t bw){
	switch(bw){
		case WIFI_BW20:
			return 20;
		case WIFI_BW40:
			return 40;
		case WIFI_BW80:
			return 80;	
		case WIFI_BW160:
			return 160;
		case WIFI_BW80_BW80:
			return 161;
		default:
			return -1;
	}
}

int get_sta_info(char sta[], int size){
	wifi_ap_record_t ap_info;
	if(esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){
		char res[24];
		get_wifi_strength(ap_info.rssi, res);
		return snprintf(
			sta,
			size,
			"<h4>STA INFO</h4>\n"
			"<li>MAC: " MACSTR "</li>\n"
			"<li>SSID: %s</li>\n"
			"<li>RSSI: %s (%d) </li>\n"
			"<li>Channel: %" PRIu8 "</li>\n"
			"<li>Bandwidth: %d MHz</li>\n",
			MAC2STR(ap_info.bssid),
			ap_info.ssid,
			res, ap_info.rssi,
			ap_info.primary,
			get_wifi_bandwidth(ap_info.bandwidth)	
		);
	} else {
		return snprintf(
				sta,
				size,
				"No wifi info"
		);
	}
}
