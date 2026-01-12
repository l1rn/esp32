#include "mqtt_client.h"
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "MQTT_ANTMINER";

static esp_mqtt_client_handle_t client = NULL;

static void parse_antminer_json(const char *root){
	cJSON *json = cJSON_Parse(root);
	if(json != NULL){
		cJSON *stats = cJSON_GetObjectItem(json, "STATS");
		cJSON *first = cJSON_GetArrayItem(stats, 0);
		cJSON *elapsed = cJSON_GetObjectItem(first, "elapsed");
		ESP_LOGI(TAG, "%s\n", cJSON_Print(elapsed));
		cJSON_Delete(json);
	}
}

static void mqtt_event_handler(void *handler_args,
				esp_event_base_t base,
				int32_t event_id,
				void *event_data){
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)event->client;

	switch(event_id){
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT Connected");
			esp_mqtt_client_subscribe(client, "antminer/stats", 0);
			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT Disconnected");
			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "Data received");
			parse_antminer_json(event->data);
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
