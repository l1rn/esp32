idf_component_register(
	SRCS "wifi_component.c" 
	INCLUDE_DIRS "."
	REQUIRES 
		esp_wifi
		nvs_flash
)
