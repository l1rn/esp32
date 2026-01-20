#include "mqtt_client.h"
#include "antminer.h"
#include "cJSON.h"
#include "esp_log.h"
#include "i2c_display.h"

static const char *TAG = "MQTT_ANTMINER";

static esp_mqtt_client_handle_t client = NULL;

miner_response_t miner_data = {0};

int parse_antminer_json(const char *root, miner_response_t *data){
	cJSON *json = cJSON_Parse(root);
	if(!root){
		ESP_LOGE(TAG, "JSON parse error\n");
		return -1;
	}

	if(json != NULL){
		cJSON *status = cJSON_GetObjectItem(json, "STATUS");
		if(status){
			strncpy(data->status, 
				cJSON_GetObjectItem(status, "STATUS")->valuestring, 
				sizeof(data->status));
			data->timestamp = cJSON_GetObjectItem(status, "when")->valueint;
		}

		cJSON *stats = cJSON_GetObjectItem(json, "STATS");
		if(stats){
			cJSON *obj = cJSON_GetArrayItem(stats, 0);
		
			cJSON *rate_avg = cJSON_GetObjectItem(obj, "rate_5s");
			cJSON *rate_unit = cJSON_GetObjectItem(obj, "rate_unit");
			cJSON *fan_num = cJSON_GetObjectItem(obj, "fan_num");
			cJSON *chain_num = cJSON_GetObjectItem(obj, "chain_num");
			
			strncpy(data->rate_unit, rate_unit->valuestring, sizeof(data->rate_unit));
			data->rate_avg = rate_avg->valuedouble;
			data->fan_num = fan_num->valueint;
			data->chain_num = chain_num->valueint;

			cJSON *fans = cJSON_GetObjectItem(obj, "fan");
			for(int i = 0; i < data->fan_num && i < 4; i++){
				data->fan_speed[i] = cJSON_GetArrayItem(fans, i)->valueint;
			}

			cJSON *chains = cJSON_GetObjectItem(obj, "chain");
			for(int i = 0; i < data->chain_num && i < 3; i++){
				cJSON *chain = cJSON_GetArrayItem(chains, i);

				data->chains[i].rate_real = 
					cJSON_GetObjectItem(chain, "rate_real")->valuedouble;

				cJSON *temp_chip = cJSON_GetObjectItem(chain, "temp_chip");
				
				int tp_in = 0; 
				for(int i = 0; i < 2; i++){
					tp_in += cJSON_GetArrayItem(temp_chip, i)->valueint;
				}

				data->chains[i].temp_in_avg = tp_in / 2;

				int tp_out = 0;
				for(int i = 2; i < 4; i++){
					tp_out += cJSON_GetArrayItem(temp_chip, i)->valueint;
				}

				data->chains[i].temp_out_avg = tp_out / 2;
				
				strncpy(data->chains[i].sn,
					cJSON_GetObjectItem(chain, "sn")->valuestring,
					sizeof(data->chains[i].sn));

				data->chains[i].hw_errors = 
					cJSON_GetObjectItem(chain, "hw")->valueint;
			}
		}
	}

	cJSON_Delete(json);
	return 0;
}

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

void oled_draw_miner_info(){
	oled_clear_buffer();

	char rate_avg[12];
	sprintf(rate_avg, "%d", (int)miner_data.rate_avg);
	char hashrate[64];
	snprintf(hashrate, sizeof(hashrate), "HASHRATE:%sGH/S", 
			rate_avg);

	char chain_temperature[48];
	int chtemp_in = 0;
	int chtemp_out = 0;
	for(int i = 0; i < miner_data.chain_num && i < 3; i++){
		chtemp_in += miner_data.chains[i].temp_in_avg;
		chtemp_out += miner_data.chains[i].temp_out_avg;
	}
	snprintf(chain_temperature, sizeof(chain_temperature), 
			"IN: %d~C / OUT: %d~C", chtemp_in / 3, chtemp_out / 3);
	int y = 10;
	oled_draw_string_buffered(hashrate, 0, y);
	// oled_draw_string(chain_temperature, 0, 22);	
	y += 11;

	for(int i = 0; i < miner_data.chain_num && i < 3; i++){
		char chain_info[128];
		snprintf(chain_info, sizeof(chain_info), 
				"%d:%d/%d~ %d~ E:%d", i, (int)miner_data.chains[i].rate_real, 
				miner_data.chains[i].temp_in_avg, miner_data.chains[i].temp_out_avg,
				miner_data.chains[i].hw_errors);
		oled_draw_string_buffered(chain_info, 0, y);
		y += 11;
	}

	char fans[64] = "F:";
	int offset = strlen(fans);
	for(int i = 0; i < miner_data.fan_num && i < 4; i++){
		offset += snprintf(fans + offset, sizeof(fans) - offset, "%d/", miner_data.fan_speed[i]);
	}
	oled_draw_string_buffered(fans, 0, y);
	oled_draw_update();
}
