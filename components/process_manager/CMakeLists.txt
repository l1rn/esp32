idf_component_register(
	SRCS "process_manager.c" "button.c"
	INCLUDE_DIRS "."
	REQUIRES 
		driver
		wifi
		ntp_time
		i2c_display
		antminer
		fonts
		http_handler
	PRIV_REQUIRES esp_driver_gpio
)
