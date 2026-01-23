#include "json_parser.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "JSON_PARSER";

int parse_antminer_json(const char *root, miner_response_t *data){
	cJSON *json = cJSON_Parse(root);
	if(!root){
		ESP_LOGE(TAG, "JSON parse error\n");
		return -1;
	}

	if(json != NULL){
		cJSON *status = cJSON_GetObjectItem(json, "STATUS");
		if(status){
			strncpy(data->status, 
				cJSON_GetObjectItem(status, "STATUS")->valuestring, 
				sizeof(data->status));
			data->timestamp = cJSON_GetObjectItem(status, "when")->valueint;
		}

		cJSON *stats = cJSON_GetObjectItem(json, "STATS");
		if(stats){
			cJSON *obj = cJSON_GetArrayItem(stats, 0);
		
			cJSON *rate_avg = cJSON_GetObjectItem(obj, "rate_5s");
			cJSON *rate_unit = cJSON_GetObjectItem(obj, "rate_unit");
			cJSON *fan_num = cJSON_GetObjectItem(obj, "fan_num");
			cJSON *chain_num = cJSON_GetObjectItem(obj, "chain_num");
			
			strncpy(data->rate_unit, rate_unit->valuestring, sizeof(data->rate_unit));
			data->rate_avg = rate_avg->valuedouble;
			data->fan_num = fan_num->valueint;
			data->chain_num = chain_num->valueint;

			cJSON *fans = cJSON_GetObjectItem(obj, "fan");
			for(int i = 0; i < data->fan_num && i < 4; i++){
				data->fan_speed[i] = cJSON_GetArrayItem(fans, i)->valueint;
			}

			cJSON *chains = cJSON_GetObjectItem(obj, "chain");
			for(int i = 0; i < data->chain_num && i < 3; i++){
				cJSON *chain = cJSON_GetArrayItem(chains, i);

				data->chains[i].rate_real = 
					cJSON_GetObjectItem(chain, "rate_real")->valuedouble;

				cJSON *temp_chip = cJSON_GetObjectItem(chain, "temp_chip");
				
				int tp_in = 0; 
				for(int i = 0; i < 2; i++){
					tp_in += cJSON_GetArrayItem(temp_chip, i)->valueint;
				}

				data->chains[i].temp_in_avg = tp_in / 2;

				int tp_out = 0;
				for(int i = 2; i < 4; i++){
					tp_out += cJSON_GetArrayItem(temp_chip, i)->valueint;
				}

				data->chains[i].temp_out_avg = tp_out / 2;
				
				strncpy(data->chains[i].sn,
					cJSON_GetObjectItem(chain, "sn")->valuestring,
					sizeof(data->chains[i].sn));

				data->chains[i].hw_errors = 
					cJSON_GetObjectItem(chain, "hw")->valueint;
			}
		}
	}

	cJSON_Delete(json);
	return 0;
}

weather_response_t parse_single_forecast_json(cJSON *item){
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

	if(cJSON_IsNumber(dt_tm) && cJSON_IsNumber(temp) 
			&& cJSON_IsString(weather_main) && cJSON_IsNumber(wind_speed)){
		if(cJSON_IsString(dt_txt)){
			strcpy(forecast.datetime, dt_txt->valuestring);
		} else {
			strcpy(forecast.datetime, "yyyy-mm-dd");
		}
		strcpy(forecast.weather, weather_main->valuestring);
		forecast.dt = dt_tm->valueint;
		forecast.wind_speed = wind_speed->valuedouble;
		forecast.feels_like = (int)feels_like->valuedouble;
		forecast.temp = (int)temp->valuedouble;
	}
	return forecast;
}

weather_response_t parse_single_forecast_string(char *data){
	cJSON *item = cJSON_Parse(data);
	if(!item){
		ESP_LOGE(TAG, "\nUnable to parse single forecase item via string");
		weather_response_t empty = {0};
		return empty;
	}
	weather_response_t result = parse_single_forecast_json(item);
	cJSON_Delete(item);
	return result;
}

int parse_weather_forecast_json(cJSON *json, weather_response_t forecasts[], int max_forecasts){
	cJSON *list = cJSON_GetObjectItem(json, "list");
	if(!cJSON_IsArray(list)){
		ESP_LOGE(TAG, "No 'list' array found");
		return 0;
	}

	int list_size = cJSON_GetArraySize(list);
	int fc_count = 0;	
	
	int items_to_parse = (list_size < max_forecasts) ? list_size : max_forecasts;

	for(int i = 0; i < items_to_parse; i++){
		cJSON *item = cJSON_GetArrayItem(list, i);
		forecasts[fc_count++] = parse_single_forecast(item);
	}
	return fc_count;
}

int parse_weather_forecast_string(char *data, weather_response_t forecasts[], int max_forecasts){
	cJSON *json = cJSON_Parse(data);
	if(json == NULL){
		ESP_LOGE(TAG, "Unable to parse weather forecast through the string!");
		return 0;
	}
	
	int fc_count = parse_weather_forecast_json(json, forecasts, max_forecasts);
	cJSON_Delete(json);
	return fc_count;
}


