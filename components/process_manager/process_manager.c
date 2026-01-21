#include "driver/gpio.h"

#include "process_manager.h"
#include "wifi.h'"
#include "ntp_time.h"

void gpio_init(void){
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_PIN, 0);
}

void wifi_process(void){
	
}
