#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

typedef enum {
	IO_OK = 0,
	IO_DIGITAL_DISCONNECTED = -1,
	IO_ADC_READ_FAILED = -2,
	IO_ADC_OUT_OF_BOUNDS = -3,
} io_status_t;

void init_temperature_config(void);
void print_temperature(void);
float get_temperature(void);
io_status_t get_io_status(void);
void print_io_error(io_status_t status);

#endif // TEMPERATURE_SENSOR_H
