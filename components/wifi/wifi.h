#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H
#include "esp_err.h"

esp_err_t 	wifi_init(void);
bool 		wifi_is_connected(void);
#endif // WIFI_MODULE_H
