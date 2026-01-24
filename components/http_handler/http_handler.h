#ifndef WEATHER_H
#define WEATHER_H

#define MAX_FORECASTS 8

#include "json_parser.h"

void get_weather_15hours(weather_response_t forecasts[], int max_forecasts);
weather_response_t get_weather_current(void);
void get_miner_info(miner_response_t *data);
void get_bitcoin_price(char *result);

#endif // WEATHER_H
