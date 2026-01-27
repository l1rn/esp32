#ifndef WEATHER_H
#define WEATHER_H

#define MAX_FORECASTS 8

#include "json_parser.h"

void get_weather_15hours(weather_response_t forecasts[], int max_forecasts);
weather_response_t get_weather_current(void);
void get_miner_info(char *antminer_ip, miner_response_t *miner_info);
char *get_bitcoin_price(void);

#endif // WEATHER_H
