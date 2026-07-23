idf_component_register(
	SRCS "wifi.c"
	INCLUDE_DIRS "."
	REQUIRES 
		nvs_flash
		esp_wifi
		fonts
	PRIV_REQUIRES
		http_handler
)
