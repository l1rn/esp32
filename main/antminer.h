#ifndef ANTMINER_H
#define ANTMINER_H

typedef struct {
	char status[16];
	int timestamp;
	char rate_unit[5];
	double rate_avg;
	int fan_num;
	int chain_num;
	int fan_speed[4];
	struct {
		double rate_real;
		char sn[32];
		int temp_in_avg;
		int temp_out_avg;
		int hw_errors;
	} chains[3];
} miner_response_t;

void mqtt_antminer_start(void);
int parse_antminer_json(const char *root, miner_response_t *data);

#endif // ANTMINER_H
