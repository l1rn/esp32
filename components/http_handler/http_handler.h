#ifndef WEATHER_H
#define WEATHER_H

#define MAX_FORECASTS 8

void get_weather_15hours(weather_response_t forecasts[], int max_forecasts);
void get_weather_current(weather_response_t *forecast);
void get_miner_info(void);

#endif // WEATHER_H
