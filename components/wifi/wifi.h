#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#define DEFAULT_SCAN_LIST_SIZE CONFIG_NETWORK_SCAN_LIST_SIZE

#include "esp_wifi.h"

typedef struct {
	const char *ssid;
	const char *password;
	int16_t rssi;
	bool available;
	bool priority;
} wifi_ap_t;

void wifi_configure(void);
void wifi_cleanup(void);

void wifi_scan_array(void);

wifi_config_t wifi_get_priority(void);
void wifi_init_sta(wifi_config_t config);
bool wifi_is_connected(void);

#endif // WIFI_MODULE_H
