idf_component_register(SRCS "main.c" "wifi.c" "antminer.c" "weather.c" "i2c_display.c" "fonts.c"
                       	INCLUDE_DIRS "."
			REQUIRES esp_wifi driver esp_driver_i2c nvs_flash esp_http_client cJSON esp_rom mqtt
			PRIV_REQUIRES esp_driver_gpio mbedtls)
