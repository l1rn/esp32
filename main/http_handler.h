#ifndef WEATHER_H
#define WEATHER_H

typedef struct {
	char datetime[20];
	char weather[32];
	int temp;
	int feels_like;
	double wind_speed;
	int dt;
} weather_response_t;

void get_weather_15hours(void);
void get_weather_current(void);

#endif // WEATHER_H
