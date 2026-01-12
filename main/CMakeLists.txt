idf_component_register(SRCS "main.c" "wifi.c" "antminer.c" "weather.c"
                       	INCLUDE_DIRS "."
			REQUIRES esp_wifi driver nvs_flash esp_http_client cJSON esp_rom mqtt
			PRIV_REQUIRES esp_driver_gpio mbedtls)
