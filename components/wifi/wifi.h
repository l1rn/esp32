#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#define DEFAULT_SCAN_LIST_SIZE CONFIG_NETWORK_SCAN_LIST_SIZE

#include "esp_wifi.h"

typedef struct {
	const char *ssid;
	const char *password;
	int16_t rssi;
	bool reachable;
	bool prioritize;
} wifi_ap_t;

void wifi_init(void);
void wifi_cleanup(void);
void wifi_scan_init(void);

void wifi_scan_single_time(
		u16 ap_count, 
		u16 number, 
		wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE]);

void wifi_init_sta(void);
bool wifi_is_connected(void);

#endif // WIFI_MODULE_H
