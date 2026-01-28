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
#include "button.h"

static const char *TAG = "MAIN";

void app_main(void) {
	display_process();
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	main_loop();
}
