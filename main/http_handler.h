#ifndef WEATHER_H
#define WEATHER_H

#define MAX_FORECASTS 8

typedef struct {
	char datetime[20];
	char weather[32];
	int temp;
	int feels_like;
	double wind_speed;
	int dt;
} weather_response_t;

void get_weather_15hours(weather_response_t forecasts[], int max_forecasts);
void get_weather_current(void);

#endif // WEATHER_H
