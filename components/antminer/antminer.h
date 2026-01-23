#ifndef ANTMINER_H
#define ANTMINER_H

#include "json_parser.h"

void mqtt_antminer_start(void);
void oled_draw_miner_info(miner_response_t response);

#endif // ANTMINER_H
