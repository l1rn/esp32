#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "antminer.h"
#include "http_handler.h"
#include "wifi.h"
#include "i2c_display.h"
#include "ntp_time.h"
#include "process_manager.h"

static const char *TAG = "MAIN";

void app_main(void) {
	vTaskDelay(pdMS_TO_TICKS(1000));

	display_process();
	wifi_process();	
	
	vTaskDelay(pdMS_TO_TICKS(2000));
	

}
