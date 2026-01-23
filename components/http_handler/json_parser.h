#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "cJSON.h"

#define MAX_CHAINS 3
#define MAX_FANS 4

#define USE_STRING_PARSE 1

typedef	struct {
	double rate_real;
	char sn[32];
	int temp_in_avg;
	int temp_out_avg;
	int hw_errors;
} chain_data_t;

typedef struct {
	char status[16];
	int timestamp;
	char rate_unit[16];
	double rate_avg;
	int fan_num;
	int chain_num;
	int fan_speed[MAX_FANS];
	chain_data_t chains[MAX_CHAINS];
} miner_response_t;

typedef struct {
	char datetime[20];
	char weather[32];
	int temp;
	int feels_like;
	double wind_speed;
	int dt;
} weather_response_t;

weather_response_t parse_single_forecast_json(cJSON *item);
weather_response_t parse_single_forecast_string(char *data);

int parse_weather_forecast_json(cJSON *json, weather_response_t forecasts, int max_forecasts);
int parse_weather_forecast_string(char *data, weather_response_t forecasts, int max_forecasts);

int parse_antminer_json(const char *root, miner_response_t *data);

/* GENERICS */
#define parse_single_forecast(x) _Generic((x), \
		cJSON*: parse_single_forecast_json, \
		char*: parse_single_forecast_string, \
		const char*: parse_single_forecast_string \
	)(x)

#define parse_weather_forecast(x) _Generic((x), \
		cJSON*: parse_weather_forecast_json, \
		char*: parse_weather_forecast_string, \
		const char*: parse_weather_forecast_string \
	)(x)
#endif // JSON_PARSER_H
