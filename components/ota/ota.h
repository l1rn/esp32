#ifndef OTA_SERVER_H
#define OTA_SERVER_H

#include "esp_http_server.h"
#include "esp_err.h"

esp_err_t register_ota_uri(httpd_handle_t server);

#endif // OTA_SERVER_H
