#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"

#include "http_handler.h"
#include "antminer.h"

#define API_KEY CONFIG_WEATHER_API_KEY
#define CITY	CONFIG_WEATHER_CITY

#define MARKET_API_KEY CONFIG_COIN_MARKET_API_KEY 

static const char *TAG = "HTTP_HANDLER";

typedef struct {
	char *data;
	size_t size;
} http_response_t;

esp_err_t http_event_handler(esp_http_client_event_t *evt){
	http_response_t *resp = (http_response_t *)evt->user_data;
	
	switch(evt->event_id){
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
        	case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
		    	break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
        	case HTTP_EVENT_ON_HEADERS_COMPLETE:
            		ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
            		break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

			int status_code = esp_http_client_get_status_code(evt->client);

			if(status_code == 401) {
				ESP_LOGI(TAG, "Skipping 401 http code");	
				return ESP_OK;
			}

			if(status_code == 200){
				resp->data = realloc(resp->data, resp->size + evt->data_len + 1);
				memcpy(resp->data + resp->size, evt->data, evt->data_len);
				resp->size += evt->data_len;
				resp->data[resp->size] = '\0';
			}

		    	break;

		case HTTP_EVENT_ON_FINISH:
		    	ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
		   	 break;
		case HTTP_EVENT_DISCONNECTED:
		    	ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
		    	break;
 
		default:
			break;
	}

	return ESP_OK;
}

weather_response_t get_weather_current(void){
	http_response_t resp = { .data = NULL, .size = 0 };
	weather_response_t weather = {0};

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
		.user_data = &resp,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t res = esp_http_client_perform(client);

	if(res == ESP_OK) {
		weather_response_t w = parse_single_forecast(resp.data);
		weather = w;
	}
	esp_http_client_cleanup(client);
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

miner_response_t get_miner_info(char *antminer_ip){
	miner_response_t m = {0};
	http_response_t data = { .data = NULL, .size = 0 };

	char url[256];
	snprintf(url, sizeof(url), "http://root:root@%s/cgi-bin/stats.cgi", antminer_ip);
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = http_event_handler,
		.user_data = &data,
		.auth_type = HTTP_AUTH_TYPE_DIGEST,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_header(client, "Accept", "application/json");
	
	esp_err_t err = esp_http_client_prepare(client);
	err = esp_http_client_perform(client);
	int status = esp_http_client_get_status_code(client); 

	if(err == ESP_OK && status == 200){
		parse_antminer_json(data.data, &m);
	} 

	esp_http_client_cleanup(client);
	return m;
}

char *get_bitcoin_price(void){
	static char result[24];
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
	return result;
}
