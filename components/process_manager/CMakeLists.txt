idf_component_register(
	SRCS "process_manager.c" 
	INCLUDE_DIRS "."
	REQUIRES 
	#		driver
		wifi_component
		temperature_sensor
		http_server
		ota
		esp_wifi
#		ntp_time
#		i2c_display
#		antminer
#		fonts
#		http_handler
	PRIV_REQUIRES 
#		esp_driver_gpio
		esp_http_server
	EMBED_FILES
		"blink.bin"
)
