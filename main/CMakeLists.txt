idf_component_register(SRCS "main.c" "wifi.c"
                       	INCLUDE_DIRS "."
			REQUIRES esp_wifi driver nvs_flash
			PRIV_REQUIRES esp_driver_gpio)
