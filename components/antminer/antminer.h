#ifndef ANTMINER_H
#define ANTMINER_H

#define MAX_CHAINS 3
#define MAX_FANS 4

#include "json_parser.h"

void mqtt_antminer_start(void);
void oled_draw_miner_info(miner_response_t response);

#endif // ANTMINER_H
