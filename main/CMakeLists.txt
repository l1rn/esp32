idf_component_register(
	SRCS 
		"main.c"
	INCLUDE_DIRS "."
	REQUIRES
		antminer
		http_handler 
		wifi
		ntp_time
		i2c_display
		fonts
		process_manager
		esp_wifi 
		esp_rom 
)
