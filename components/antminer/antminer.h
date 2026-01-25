#ifndef ANTMINER_H
#define ANTMINER_H

#include "json_parser.h"

void mqtt_antminer_start(void);
void oled_draw_miner_info(miner_response_t response, char *btc);
miner_response_t get_miner_data(void);

#endif // ANTMINER_H
