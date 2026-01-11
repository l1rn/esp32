#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "cJSON.h"

#define API_KEY CONFIG_WEATHER_API_KEY
#define CITY	CONFIG_WEATHER_CITY

static const char *TAG = "WEATHER";

typedef struct {
	char *data;
	size_t size;
} http_response_t;

esp_err_t http_event_handler(esp_http_client_event_t *evt){
	http_response_t *resp = (http_response_t *)evt->user_data;
	
	if(evt->event_id == HTTP_EVENT_ON_DATA){
		resp->data = realloc(resp->data, resp->size + evt->data_len + 1);
		memcpy(resp->data + resp->size, evt->data, evt->data_len);
		resp->size += evt->data_len;
		resp->data[resp->size] = '\0';
	}

	return ESP_OK;
}

void get_weather(void){
	http_response_t response = {
		.data = NULL,
		.size = 0
	};

	char url[256];
	snprintf(
			url, 
			sizeof(url), 
			"http://api.openweathermap.org/data/2.5/weather?q=%s&units=metric&appid=%s", 
			CONFIG_WEATHER_CITY,
			CONFIG_WEATHER_API_KEY
		);

	esp_http_client_config_t config = {
		.url = 			url,
		.event_handler =	http_event_handler,
		.user_data =		&response,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t 		 res	= esp_http_client_perform(client);
	if(res == ESP_OK){
		cJSON *json = cJSON_Parse(response.data);
		if(json == NULL){
			printf("Error parsing JSON");
		} else {
			cJSON *name 	= cJSON_GetObjectItem(json, "name");
			cJSON *main_obj = cJSON_GetObjectItem(json, "main");
			cJSON *temp	= cJSON_GetObjectItem(main_obj, "temp");

			if(cJSON_IsString(name) && cJSON_IsNumber(temp)){
				ESP_LOGI(TAG, "City: %s, Temp: %f\n", name->valuestring, temp->valuedouble);
			}

			cJSON_Delete(json);
		}
	}
	esp_http_client_cleanup(client);
	free(response.data);
}

