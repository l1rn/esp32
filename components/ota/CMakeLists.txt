idf_component_register(
	SRCS "ota.c" "espnow_ota_server.c"
	INCLUDE_DIRS "."
	REQUIRES 
		app_update esp_http_server common_types esp_timer esp_wifi
)
