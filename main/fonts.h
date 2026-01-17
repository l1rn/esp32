#ifndef FONTS_H
#define FONTS_H
#include "stdint.h"

extern const uint8_t font_5x7[96][5];

#define GET_CHAR_DATA(c) font_5x7[(c) - 32]

#endif
