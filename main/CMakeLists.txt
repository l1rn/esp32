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
		esp_wifi 
		driver 
		esp_rom 
	PRIV_REQUIRES 
		esp_driver_gpio 
)
