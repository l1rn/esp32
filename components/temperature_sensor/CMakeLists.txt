idf_component_register(
	SRCS "temperature_sensor.c"
	INCLUDE_DIRS "."
	REQUIRES esp_adc
	PRIV_REQUIRES esp_driver_gpio
)
