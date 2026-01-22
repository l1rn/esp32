#ifndef COLORS_H
#define COLORS_H

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

#endif // COLORS_H
