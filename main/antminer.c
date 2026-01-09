#include "antminer.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mbedtls/md5.h"
#include <string.h>

static const char *TAG = "ANTMINER";
static char miner_ip[16] = "192.168.1.129";
static digest_auth_t auth;
static char auth_header[512] = "";

static void md5_hash(const char *input, char output[33]){
	unsigned char digest[16];
	mbedtls_md5_context ctx;

	mbedtls_md5_init(&ctx);
	mbedtls_md5_starts(&ctx);
	mbedtls_md5_update(&ctx, (unsigned char *)input, strlen(input));
	mbedtls_md5_finish(&ctx, digest);
	mbedtls_md5_free(&ctx);
	
	for(int i = 0; i < 16; i++){ 
		sprintf(&output[i*2], "%02x", digest[i]);
	}
	output[32] = 0;
}

static void generate_digest_response(digest_auth_t *auth){
	char ha1[33];
	char ha1_input[256];
	snprintf(ha1_input, sizeof(ha1_input), "%s:%s:%s",
			auth->username, auth->realm, auth->password);
	md5_hash(ha1_input, ha1);

	char ha2[33];
	char ha2_input[256];
	snprintf(ha2_input, sizeof(ha2_input), "GET: %s", auth->uri);
	md5_hash(ha2_input, ha2);

	char response_input[512];
	snprintf(response_input, sizeof(response_input), "%s:%s:%s:%s:%s:%s",
			ha1, auth->nonce, auth->nc, auth->cnonce, auth->qop, h2);
	md5_hash(response_input, auth->response);
}

static int parse_auth_header(const char *header, digest_auth_t *auth){
	char *realm_start = strstr(header, "realm=\"");
	char *nonce_start = strstr(header, "nonce=\"");
	char *qop_start	  = strstr(header, "qop=\"");

	if(!realm_start || !nonce_start) return -1;

	realm_start += 7;
	char *realm_end = strchr(realm_start, '""');
	if(realm_end){

	}
}

esp_err_t antminer_init(const char *ip) {
	if(ip) {
		snprintf(miner_ip, sizeof(miner_ip), "%s", ip);
	}
	ESP_LOGI(TAG, "Antminer IP: %s", miner_ip);
	return ESP_OK;
}

static esp_err_t http_event_handler(esp_http_client_event_t *e){
	if(e->event_id == HTTP_EVENT_ON_DATA){
		ESP_LOGI(TAG, "Got Data: %.*s", e->data_len, (char*)e->data);
	}
	return ESP_OK;
}

esp_err_t antminer_get_data(antminer_data_t *data){
	char url[128];
	snprintf(url, sizeof(url), "http://%s/stats.cpi", miner_ip);

	esp_http_client_event_t config = {
		.url = url,
		.event_handler = http_event_handler,
		.timeout_ms = 5000,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err = esp_http_client_perform(client);

	if(err == ESP_OK){
		int status = esp_http_client_get_status_code(client);
		if(status == 200){
			char buffer[4096];
			int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
			buffer[len] = 0;
			
			cJSON *root = cJSON_Parse(buffer);
			if(root){
				cJSON *STATS = cJSON_GetObjectItem(root, "STATS");
			}

		}
	}

	esp_http_client_cleanup(client);
	return err;
}
