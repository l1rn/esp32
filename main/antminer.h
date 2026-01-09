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

typedef struct {
	char username[8];
	char password[8];
	char realm[64];
	char none[64];
	char uri[128];
	char response[64];
	char qop[16];
	char np[16];
	char cnonce[32];
} digest_auth_t;

esp_err_t antminer_init(const char *ip, const char *user, const char *pass);
esp_err_t antminer_get_data(antminer_data_t *data);

#endif
