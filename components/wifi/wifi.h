#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#define DEFAULT_SCAN_LIST_SIZE CONFIG_NETWORK_SCAN_LIST_SIZE

#include "esp_wifi.h"

typedef uint16_t u16;
typedef uint8_t u8;

void wifi_init(void);
void wifi_scan_init(void);

void wifi_scan_single_time(
		u16 ap_count, 
		u16 number, 
		wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE]);

void wifi_connect(void);
bool wifi_is_connected(void);

#endif // WIFI_MODULE_H
