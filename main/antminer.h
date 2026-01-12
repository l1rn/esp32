#ifndef ANTMINER_H
#define ANTMINER_H

#include "esp_err.h"

typedef struct {
	float hashrate;
	int temp;
	int fanspeed;
	int uptime;
	char pool[64];
} antminer_data_t;

void antminer_get_data(void);

#endif
