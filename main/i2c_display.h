#ifndef I2C_DISPLAY_H
#define I2C_DISPLAY_H

esp_err_t i2c_init(void);
int scan_i2c(void);
void i2c_procedure(void);

#endif // I2C_DISPLAY_H
