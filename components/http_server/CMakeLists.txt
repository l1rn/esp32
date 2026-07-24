idf_component_register(
	SRCS "http_server.c"
	INCLUDE_DIRS "."
	REQUIRES temperature_sensor
	PRIV_REQUIRES esp_http_server
)
