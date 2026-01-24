#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"

#include "http_handler.h"
#include "antminer.h"

#define API_KEY CONFIG_WEATHER_API_KEY
#define CITY	CONFIG_WEATHER_CITY

#define ANTMINER_IP CONFIG_ANTMINER_IP_2 
#define MAX_HTTP_OUTPUT_BUFFER 2048

#define MARKET_API_KEY CONFIG_COIN_MARKET_API_KEY 

static const char *TAG = "HTTP_HANDLER";

typedef struct {
	char *data;
	size_t size;
} http_response_t;

esp_err_t http_event_handler(esp_http_client_event_t *evt){
	static char *output_buffer;
	static int output_len;
	http_response_t *resp = (http_response_t *)evt->user_data;
	
	if(evt->event_id == HTTP_EVENT_ON_DATA){
		ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);	
		resp->data = realloc(resp->data, resp->size + evt->data_len + 1);
		memcpy(resp->data + resp->size, evt->data, evt->data_len);
	
		resp->size += evt->data_len;
		resp->data[resp->size] = '\0';
	}

	return ESP_OK;
}

weather_response_t get_weather_current(void){
	weather_response_t weather = {0};
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
		weather_response_t w = parse_single_forecast(response.data);
		weather = w;
	}
	esp_http_client_cleanup(client);
	free(response.data);
	return weather;
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
		int count = parse_weather_forecast(response.data, forecasts, max_forecasts);
		for (int i = 0; i < count; i++) {
		    ESP_LOGI("WEATHER", "%s: %s, %d°C (feels %d), wind %d m/s",
			     forecasts[i].datetime,
			     forecasts[i].weather,
			     forecasts[i].temp,
			     forecasts[i].feels_like,
			     forecasts[i].wind_speed);
		}
	}
	esp_http_client_cleanup(client);
	free(response.data);
}

void get_miner_info(miner_response_t *miner_data){
	http_response_t data = { .data = NULL, .size = 0 };
	char url[256];
	snprintf(url, sizeof(url), "http://root:root@%s/cgi-bin/stats.cgi", 
			ANTMINER_IP);
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = http_event_handler,
		.user_data = &data,
		.auth_type = HTTP_AUTH_TYPE_DIGEST,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err = esp_http_client_perform(client);

	if(err == ESP_OK){
		ESP_LOGI(TAG, "http md5 digest auth status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		c_print(RED, "RESPONSE DATA SIZE: %d", data.size);
	} else {
		ESP_LOGE(TAG, "Error performing http request: %s", esp_err_to_name(err));
	}

	esp_http_client_cleanup(client);
}

void get_bitcoin_price(char *result){
	http_response_t response = { .data = NULL, .size = 0 };
	esp_http_client_config_t config = {
		.url = "https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?symbol=BTC&convert=USD",
		.event_handler = http_event_handler,
		.user_data = &response,
		.crt_bundle_attach = esp_crt_bundle_attach,
		.method = HTTP_METHOD_GET,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_header(client, "X-CMC_PRO_API_KEY", MARKET_API_KEY);
	esp_http_client_set_header(client, "Accept", "application/json");

	esp_err_t err = esp_http_client_perform(client);
	if(err == ESP_OK){
		ESP_LOGI(TAG, "bitcoin requets status code: %d", esp_http_client_get_status_code(client));

		parse_bitcoin_price(response.data, result);
	}

	esp_http_client_cleanup(client);
}
