#ifndef I2C_DISPLAY_H
#define I2C_DISPLAY_H

void i2c_master_init(i2c_master_bus_handle_t *bus);
void scan_i2c(i2c_master_bus_handle_t bus);

#endif // I2C_DISPLAY_H
