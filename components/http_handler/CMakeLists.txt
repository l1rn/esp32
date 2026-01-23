idf_component_register(
	SRCS 
		"http_handler.c" "json_parcer.c"
	INCLUDE_DIRS 
		"."
	REQUIRES 
		esp_http_client 
		cJSON
		antminer
)
