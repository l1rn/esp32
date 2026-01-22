idf_component_register(
	SRCS "process_manager.c"
	INCLUDE_DIRS "."
	REQUIRES 
		driver
		wifi
		ntp_time
		i2c_display
	PRIV_REQUIRES esp_driver_gpio
)
