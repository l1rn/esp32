idf_component_register(
	SRCS "process_manager.c"
	INCLUDE_DIRS "."
	REQUIRES 
		driver
		wifi
	PRIV_REQUIRES esp_driver_gpio
)
