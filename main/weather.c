#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

void format_date_dd_mm(const char *input, char *output, size_t out_size){
	struct tm tm = {0};

	if(strptime(input, "%Y-%m-%d %H:%M:%S", &tm) == NULL){
		return;
	}

	strftime(output, out_size, "%d-%b", &tm);
}

void get_weather_current(void){
	http_response_t response = {
		.data = NULL,
		.size = 0
	};

	char url[256];
	snprintf(
			url,
			sizeof(url),
			"http://api.openweathermap.org/data/2.5/weather?/q%s&units=metric&appid=%s",
			CONFIG_WEATHER_CITY,
			CONFIG_WEATHER_API_KEY
		);

	esp_http_client_config_t config = {
		.url = url,
		.event_handler = http_event_handler,
		.user_data = &response
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t res = esp_http_client_perform(client);
	if(res == ESP_OK) {
		cJSON *json = cJSON_Parse(response.data);
		if(json == NULL){
			ESP_LOGE(TAG, "Error parsing json");
			return;
		} else {
			cJSON *name = cJSON_GetObjectItem(json, "name");

			cJSON *weather_obj = cJSON_GetObjectItem(json, "weather");
			cJSON *weather = cJSON_GetArrayItem(weather_obj, 0);
			cJSON *weather_name = cJSON_GetObjectItem(weather, "main");
			
			cJSON *main_obj = cJSON_GetObjectItem(json, "main");
			cJSON *temp = cJSON_GetObjectItem(main_obj, "temp");
			cJSON *feels_like = cJSON_GetObjectItem(main_obj, "feels_like");

			cJSON *wind_obj = cJSON_GetObjectItem(json, "wind");
			cJSON *wind_speed = cJSON_GetObjectItem(wind_obj, "speed");
			if(cJSON_IsNumber(temp) && cJSON_IsNumber(wind_speed) && cJSON_IsNumber(feels_like)){
				ESP_LOGI(TAG, "temp: %f, feels like: %f, wind speed: %f", 
					temp->valuedouble, feels_like->valuedouble, wind_speed->valuedouble);
			}
		}

		cJSON_Delete(json);
	}
	esp_http_client_cleanup(client);
	free(response.data);
}

void get_weather_15hours(void){
	http_response_t response = {
		.data = NULL,
		.size = 0
	};

	char url[256];
	snprintf(
			url, 
			sizeof(url), 
			"http://api.openweathermap.org/data/2.5/forecast?q=%s&units=metric&cnt=5&appid=%s", 
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
			
			cJSON *city_obj	= cJSON_GetObjectItem(json, "city");
			cJSON *list_obj = cJSON_GetObjectItem(json, "list");
			int size = cJSON_GetArraySize(list_obj);
			for(int i = 0; i < size; i++){
				cJSON *list = cJSON_GetArrayItem(list_obj, i);
				
				cJSON *date_txt = cJSON_GetObjectItem(list, "dt_txt");
				cJSON *main_obj = cJSON_GetObjectItem(list, "main");
				cJSON *weather_obj = cJSON_GetObjectItem(list, "weather");
				cJSON *weather = cJSON_GetArrayItem(weather_obj, 0);
				cJSON *wind_obj = cJSON_GetObjectItem(list, "wind");
				
				cJSON *temp = cJSON_GetObjectItem(main_obj, "temp");
				cJSON *feels_like = cJSON_GetObjectItem(main_obj, "feels_like");
				
				cJSON *weather_txt = cJSON_GetObjectItem(weather, "main");
				cJSON *wind_speed = cJSON_GetObjectItem(wind_obj, "speed");			

				char date_buf[16];
				if(cJSON_IsNumber(temp) && cJSON_IsNumber(feels_like)){
					ESP_LOGI(TAG, "temp: %f\n", temp->valuedouble);
					ESP_LOGI(TAG, "feelslike: %f\n", feels_like->valuedouble);
					ESP_LOGI(TAG, "wind speed: %f\n", wind_speed->valuedouble);
					ESP_LOGI(TAG, "date: %s\n", date_txt->valuestring);
				}
			}
			cJSON_Delete(json);
		}
	}
	esp_http_client_cleanup(client);
	free(response.data);
}
