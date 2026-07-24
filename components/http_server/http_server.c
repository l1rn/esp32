#include "esp_log.h"
#include "esp_err.h"
#include <stdio.h>
#include "http_server.h"
#include "temperature_sensor.h"

static const char *TAG = "HTTP_SERVER";

static esp_err_t root_get_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, "<h1>Hello from esp32!</h1>", HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}


static const httpd_uri_t root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = root_get_handler
};

static esp_err_t temp_sensor_get_handler(httpd_req_t *req){
	httpd_resp_set_type(req, "application/json");
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "{%.2f}", get_temperature());
	httpd_resp_send(req, "", HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

static const httpd_uri_t temperature_sensor = {
	.uri = "/temperature-sensor",
	.method = HTTP_GET,
	.handler = temp_sensor_get_handler
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

