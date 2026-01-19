#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "http_handler.h"

#include "cJSON.h"

#define API_KEY CONFIG_WEATHER_API_KEY
#define CITY	CONFIG_WEATHER_CITY

static const char *TAG = "HTTP_HANDLER";

typedef struct {
	char *data;
	size_t size;
} http_response_t;

weather_response_t parse_single_forecast(cJSON *item){
	weather_response_t forecast = {0};

	cJSON *dt_txt = cJSON_GetObjectItem(item, "dt_txt");
	cJSON *dt_tm = cJSON_GetObjectItem(item, "dt");

	cJSON *main = cJSON_GetObjectItem(item, "main");
	cJSON *temp = cJSON_GetObjectItem(main, "temp");
	cJSON *feels_like = cJSON_GetObjectItem(main, "feels_like");
	
	cJSON *weather = cJSON_GetObjectItem(item, "weather");
	cJSON *weather_item = cJSON_GetArrayItem(weather, 0);
	cJSON *weather_main = cJSON_GetObjectItem(weather_item, "description");
	
	cJSON *wind = cJSON_GetObjectItem(item, "wind");
	cJSON *wind_speed = cJSON_GetObjectItem(wind, "speed");			

	if(cJSON_IsString(dt_txt) && cJSON_IsNumber(dt_tm) && cJSON_IsNumber(temp) 
			&& cJSON_IsString(weather_main) && cJSON_IsNumber(wind_speed)){
		strcpy(forecast.datetime, dt_txt->valuestring);
		strcpy(forecast.weather, weather_main->valuestring);
		forecast.dt = dt_tm->valueint;
		forecast.wind_speed = wind_speed->valuedouble;
		forecast.feels_like = (int)feels_like->valuedouble;
		forecast.temp = (int)temp->valuedouble;
	}
	return forecast;
}

int parse_weather_forecast(cJSON *json, weather_response_t forecasts[], int max_forecasts){
	cJSON *list = cJSON_GetObjectItem(json, "list");
	if(!cJSON_IsArray(list)){
		ESP_LOGE(TAG, "No 'list' array found");
		return 0;
	}

	int list_size = cJSON_GetArraySize(list);
	int fc_count = 0;	
	
	int items_to_parse = (list_size < max_forecasts) ? list_size : max_forecasts;

	for(int i = 0; i < list_size; i++){
		cJSON *item = cJSON_GetArrayItem(list, i);
		forecasts[fc_count++] = parse_single_forecast(item);
	}
	return fc_count;
}

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

void get_weather_current(weather_response_t *forecast){
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
			weather_response_t tmp = parse_single_forecast(json);
			forecast->temp = tmp.temp;
			forecast->feels_like = tmp.feels_like;
			cJSON_Delete(json);
		}
	}
	esp_http_client_cleanup(client);
	free(response.data);
}

void get_weather_15hours(weather_response_t forecasts[], int max_forecasts){
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
			int count = parse_weather_forecast(json, forecasts, max_forecasts);
			for (int i = 0; i < count; i++) {
			    ESP_LOGI("WEATHER", "%s: %s, %d°C (feels %d), wind %d m/s",
				     forecasts[i].datetime,
				     forecasts[i].weather,
				     forecasts[i].temp,
				     forecasts[i].feels_like,
				     forecasts[i].wind_speed);
			}
			cJSON_Delete(json);
		}
	}
	esp_http_client_cleanup(client);
	free(response.data);
}
