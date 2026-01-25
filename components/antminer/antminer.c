#include "mqtt_client.h"
#include "antminer.h"
#include "cJSON.h"
#include "esp_log.h"
#include "i2c_display.h"

#include "http_handler.h"

static const char *TAG = "MQTT_ANTMINER";

static esp_mqtt_client_handle_t client = NULL;

miner_response_t miner_data = {0};

static void mqtt_event_handler(void *handler_args,
				esp_event_base_t base,
				int32_t event_id,
				void *event_data){
	esp_mqtt_event_handle_t event = event_data;

	switch(event_id){
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT Connected");
			esp_mqtt_client_subscribe(event->client, "antminer/stats", 0);
			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT Disconnected");
			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "Data received, len: %d", event->data_len);
			parse_antminer_json(event->data, &miner_data);
			break;
		
		default:
			break;
	}
}

void mqtt_antminer_start(void){
	esp_mqtt_client_config_t cfg = {
		.broker.address.uri = "mqtt://192.168.1.1",
		.network.timeout_ms = 10000,
		.session.keepalive = 60,
		.buffer.size = 4096
	};

	client = esp_mqtt_client_init(&cfg);
	esp_mqtt_client_register_event(
			client,
			ESP_EVENT_ANY_ID,
			mqtt_event_handler,
			NULL
			);
	esp_mqtt_client_start(client);
}

miner_response_t get_miner_data(void){
	return miner_data;
}

void oled_draw_miner_info(miner_response_t response, char *btc){
	oled_clear_buffer();

	char rate_avg[12];
	sprintf(rate_avg, "%d", (int)response.rate_avg);
	char hashrate[64];
	snprintf(hashrate, sizeof(hashrate), "HASHRATE:%sGH/S", 
			rate_avg);

	char chain_temperature[48];
	int chtemp_in = 0;
	int chtemp_out = 0;
	for(int i = 0; i < response.chain_num && i < 3; i++){
		chtemp_in += response.chains[i].temp_in_avg;
		chtemp_out += response.chains[i].temp_out_avg;
	}
	snprintf(chain_temperature, sizeof(chain_temperature), 
			"IN: %d~C / OUT: %d~C", chtemp_in / 3, chtemp_out / 3);
	int y = 10;
	oled_draw_string_buffered(hashrate, 0, y);
	y += 11;

	for(int i = 0; i < response.chain_num && i < 3; i++){
		char chain_info[128];
		snprintf(chain_info, sizeof(chain_info), 
				"%d:%d/%d~ %d~ E:%d", i, (int)response.chains[i].rate_real, 
				response.chains[i].temp_in_avg, response.chains[i].temp_out_avg,
				response.chains[i].hw_errors);
		oled_draw_string_buffered(chain_info, 0, y);
		y += 11;
	}
	oled_draw_string_buffered(btc, 0, y);

	oled_draw_update();
}
