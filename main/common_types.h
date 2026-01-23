#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#ifndef __ASSEMBLER__

#include <stdint.h>

#define RST "\033[0m"
#define RED "\033[31m"
#define GRN "\033[32m"
#define YEL "\033[33m"
#define BLU "\033[34m"
#define MAG "\033[35m"
#define CYN "\033[36m"
#define WHT "\033[37m"

#define LGBL_WHT "\033[0;104m"

#define c_print(color, format, ...) printf(color format "%s", ##__VA_ARGS__, RST)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;

#endif // __ASSEMBLER__

#endif // COMMON_TYPES_H
