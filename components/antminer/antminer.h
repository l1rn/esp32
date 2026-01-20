#ifndef ANTMINER_H
#define ANTMINER_H

#define MAX_CHAINS 3
#define MAX_FANS 4

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

void mqtt_antminer_start(void);
void oled_draw_miner_info(void);

#endif // ANTMINER_H
