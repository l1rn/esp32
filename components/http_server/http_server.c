#include "esp_log.h"
#include "esp_err.h"
#include <stdio.h>
#include "http_server.h"
#include "system_info.h"

static const char *TAG = "HTTP_SERVER";
static char log_history[100][64];

static esp_err_t root_get_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");
	char uptime[64];
	int uptime_len = get_uptime(uptime, sizeof(uptime));
	char info[256];
	int info_len = get_system_info(info, sizeof(info));
	char wifi_info[256];
	int wifi_len = get_sta_info(wifi_info, sizeof(wifi_info));
	
	httpd_resp_send_chunk(req, uptime, uptime_len);
	httpd_resp_send_chunk(req, info, info_len);
	httpd_resp_send_chunk(req, wifi_info, wifi_len);
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

static const httpd_uri_t root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = root_get_handler
};

httpd_handle_t start_server(void){
	httpd_handle_t server = NULL;

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);

	if(httpd_start(&server, &config) == ESP_OK){
		httpd_register_uri_handler(server, &root);
		return server;
	}

	ESP_LOGI(TAG, "Failed to start a server on port: %d (kill the process on this port)", config.server_port);

	return NULL;
}

