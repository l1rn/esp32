idf_component_register(
	SRCS "lvgl_port.c"
	INCLUDE_DIRS "."
	REQUIRES lvgl driver esp_driver_i2c i2c_display
)
