#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <math.h>

// #include "antminer.h"
// #include "http_handler.h"
// #include "wifi.h"
// #include "i2c_display.h"
// #include "ntp_time.h"
// #include "process_manager.h"
// #include "button.h"

static const char *TAG = "MAIN";

#define DIGITAL_IN_PIN GPIO_NUM_18
#define ANALOG_ADC_UNIT ADC_UNIT_1
#define ANALOG_ADC_CHAN ADC_CHANNEL_6

#define R_FIXED 1050000.0f
#define R0 100000.0f
#define T0 298.15f
#define B_COEFFICIENT 3950.0f

float calculate_temperature(int raw_adc){
	if (raw_adc <= 50 || raw_adc >= 4000) return -999.0f; 
	float rNtc = R_FIXED * ((float)raw_adc / (4095.0f - (float)raw_adc));

    	float steinhart;
    	steinhart = rNtc / R0;
    	steinhart = logf(steinhart);
    	steinhart /= B_COEFFICIENT;
    	steinhart += 1.0f / T0;
    	steinhart = 1.0f / steinhart;

	ESP_LOGI(TAG, "Resistence: %.02f", rNtc);
    	return steinhart - 273.15f;
}

void app_main(void) {
//	display_process();
//	vTaskDelay(1000 / portTICK_PERIOD_MS);
//	main_loop();
//	project_cleanup();

	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << DIGITAL_IN_PIN),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};

	gpio_config(&io_conf);

	adc_oneshot_unit_handle_t adc1_handle;
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ANALOG_ADC_UNIT,
	};

	adc_oneshot_new_unit(&init_config1, &adc1_handle);

	adc_oneshot_chan_cfg_t cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC_ATTEN_DB_12,
	};

	adc_oneshot_config_channel(adc1_handle, ANALOG_ADC_CHAN, &cfg);

	while(1){
		int raw_adc_val = 0;
		int digital_state = gpio_get_level(DIGITAL_IN_PIN);
		adc_oneshot_read(adc1_handle, ANALOG_ADC_CHAN, &raw_adc_val);
		printf("A0 (Raw ADC): %d | D0 (Digital Trigger): %d\n", raw_adc_val, digital_state);
		ESP_LOGI(TAG, "Real temperature: %.2f", calculate_temperature(raw_adc_val));
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
