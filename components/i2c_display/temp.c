#include "temp.h"

#include <math.h>
#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define KY028_ADC_CHANNEL ADC_CHANNEL_6
#define KY028_ADC_UNIT ADC_UNIT_1

#define SERIES_RESISTOR 10000.0
#define NOMINAL_RESISTOR 100000.0
#define NOMINAL_TEMP 298.15
#define B_COEFFICIENT 3950.0

const static char* TAG = "TEMPERATURE SENSOR";

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t cali_handle = NULL;

float adc_to_temperature(int raw){
	if (raw >= 4095) raw = 4094;
	if (raw <= 1) raw = 1;

	float resistence = SERIES_RESISTOR;
	
	float steinhart;
	steinhart = resistence / NOMINAL_RESISTOR;
	steinhart = log(steinhart);
	steinhart /= B_COEFFICIENT;
	steinhart += 1.0 / NOMINAL_TEMP;
	steinhart = 1.0 / steinhart;
	steinhart -= 273.15;

	return steinhart;
}

void temp_init(void){
	ESP_LOGI(TAG, "Starting KY-028 Temperature Sensor on GPIO34");
	adc_oneshot_unit_init_cfg_t init_config = {
		.unit_id = KY028_ADC_UNIT,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

	adc_oneshot_chan_cfg_t config = {
		.atten = ADC_ATTEN_DB_12,           // 0-3.3V range
		.bitwidth = ADC_BITWIDTH_12,        // 0-4095 resolution
	};
    	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, KY028_ADC_CHANNEL, &config));

	adc_cali_line_fitting_config_t cali_config = {
		.unit_id = KY028_ADC_UNIT,
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_12,
    	};
    	ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));

    	ESP_LOGI(TAG, "ADC initialized successfully");
}

void temp_task(void *pvParameters){
	for(;;){
		int raw_value = 0;
		int voltage_mv = 0;

		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, KY028_ADC_CHANNEL, &raw_value));
        
		if (cali_handle != NULL) {
		    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv));
		}
		
		float temperature = adc_to_temperature(raw_value);
		
		ESP_LOGI(TAG, "Raw: %d, Voltage: %d mV, Temperature: %.2f °C", 
			 raw_value, voltage_mv, temperature);
		
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}	
}

void temp_cleanup(void){
	ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
	if (cali_handle != NULL) {
		ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(cali_handle));
    	}
}
