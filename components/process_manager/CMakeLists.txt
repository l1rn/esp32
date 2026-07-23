idf_component_register(
	SRCS "process_manager.c" 
	INCLUDE_DIRS "."
	REQUIRES 
	#		driver
		wifi
		temperature_sensor
		http_server
#		ntp_time
#		i2c_display
#		antminer
#		fonts
#		http_handler
	PRIV_REQUIRES 
#		esp_driver_gpio
		esp_http_server
)
