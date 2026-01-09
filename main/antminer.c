#include "antminer.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include "mbedtls/md.h"

static const char *TAG = "ANTMINER";
static char miner_ip[16] = "192.168.1.129";
static digest_auth_t auth;
static char auth_header[512] = "";

static void md5_hash(const char *input, char output[33]){
	unsigned char digest[16];
	mbedtls_md_context_t ctx;
	mbedtls_md_type_t md_type = MBEDTLS_MD_MD5;
    
    	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    	mbedtls_md_starts(&ctx);
    	mbedtls_md_update(&ctx, (unsigned char*)input, strlen(input));
    	mbedtls_md_finish(&ctx, digest);
    	mbedtls_md_free(&ctx);	
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
	snprintf(ha2_input, sizeof(ha2_input), "GET:%s", auth->uri);
	md5_hash(ha2_input, ha2);

	char response_input[512];
	snprintf(response_input, sizeof(response_input), "%s:%s:%s:%s:%s:%s",
			ha1, auth->nonce, auth->nc, auth->cnonce, auth->qop, ha2);
	md5_hash(response_input, auth->response);
}

static int parse_auth_header(const char *header, digest_auth_t *auth){
	char *realm_start = strstr(header, "realm=\"");
	char *nonce_start = strstr(header, "nonce=\"");
	char *qop_start	  = strstr(header, "qop=\"");

	if(!realm_start || !nonce_start) return -1;

	realm_start += 7;
	char *realm_end = strchr(realm_start, '"');
	if(realm_end){
		int len = realm_end - realm_start;
		strncpy(auth->realm, realm_start, len);
		auth->realm[len] = 0;
	}

	nonce_start += 7;
	char *nonce_end = strchr(nonce_start, '"');
	if(nonce_end){
		int len = nonce_end - nonce_start;
		strncpy(auth->nonce, nonce_start, len);
		auth->nonce[len] = 0;
	}
	
	if(qop_start){
		qop_start += 5;
		char *qop_end = strchr(qop_start, '"');
		if(qop_end){
			int len = qop_end - qop_start;
			strncpy(auth->qop, qop_start, len);
			auth->qop[len] = 0;
		}
	} else {
		strcpy(auth->qop, "auth");
	}

	snprintf(auth->cnonce, sizeof(auth->cnonce), "%08lx", esp_random());
	static uint32_t nc_counter = 1;
	snprintf(auth->nc, sizeof(auth->nc), "%08x", nc_counter++);

	return 0;
}

static void create_digest_header(digest_auth_t *auth, const char *uri){
	strcpy(auth->uri, uri);
	generate_digest_response(auth);

	snprintf(auth_header, sizeof(auth_header),
			"Digest u``sername=\"%s\", realm=\"%s\", "
			"nonce=\"%s\", uri=\"%s\", response=\"%s\", "
			"qop=%s, nc=%s, cnonce=\"%s\"", 
			auth->username, auth->realm, auth->nonce, auth->uri,
			auth->response, auth->qop, auth->nc, auth->cnonce);

	ESP_LOGI(TAG, "Digest header created for URI: %s", uri);
}

esp_err_t antminer_init(const char *ip, const char *user, const char *pass) {
	if(ip) snprintf(miner_ip, sizeof(miner_ip), "%s", ip);

	strncpy(auth.username, user ? user : "root", sizeof(auth.username - 1));
	auth.username[sizeof(auth.usename) - 1] = 0;
	strncpy(auth.password, pass ? pass : "root", sizeof(auth.password - 1));
	auth.password[sizeof(auth.password) - 1] = 0;

	ESP_LOGI(TAG, "Antminer IP: %s, User: %s", miner_ip, auth.username);
	return ESP_OK;
}

static void parse_digest_challenge(const char *h){
	sscanf(strstr(h, "realm=\"") + 7, "%63[^\"]", auth.realm);
	sscanf(strstr(h, "nonce=\"") + 7, "%63[^\"]", auth.nonce);

	if(strstr(h, "qop=\"auth\"")) {
		strcpy(auth.qop, "auth");
	}
	ESP_LOGI(TAG, "Parsed realm=%s nonce=%s qop=%s",
             auth.realm, auth.nonce, auth.qop);
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
	if(evt->event_id == HTTP_EVENT_ON_HEADER){
		if(strcasecmp(evt->header_key, "WWW-Authenticate") == 0){
			ESP_LOGI(TAG, "Auth header: %s", evt->header_value);
			parse_digest_challenge(evt->header_value);
		}
	}
	return ESP_OK;
}

static esp_err_t get_auth_challenge(void){
	char url[128];
	snprintf(url, sizeof(url), "http://%s/cgi-bin/stats.cgi", miner_ip);

	esp_http_client_config_t config = {
	    	.url = url,
    		.timeout_ms = 5000,
		.disable_auto_redirect = true,
		.event_handler = http_event_handler,
	};


	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err = esp_http_client_perform(client);
	int status = esp_http_client_get_status_code(client);
	if (status != 401) {
    		ESP_LOGE(TAG, "Expected 401, got %d", status);
    		return ESP_FAIL;
	

	esp_http_client_cleanup(client);
	if(strlen(auth.nonce) == 0 || strlen(auth.realm) == 0){
		return ESP_FAIL;
	}
	return err;
}

esp_err_t antminer_get_data(antminer_data_t *data){
	if(strlen(auth.nonce) == 0){
		ESP_LOGI(TAG, "Getting auth challenge...");
		if(get_auth_challenge() != ESP_OK){
			ESP_LOGE(TAG, "Auth challenge failed (network not ready?)");
			return ESP_FAIL;
		}
	}
	
	char url[128];
	snprintf(url, sizeof(url), "http://%s/cgi-bin/stats.cgi", miner_ip);
	
	create_digest_header(&auth, "/cgi-bin/stats.cgi");


	esp_http_client_config_t config = {
		.url = url,
		.timeout_ms = 5000,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	
	esp_http_client_set_header(client, "Authorization", auth_header);

	esp_err_t err = esp_http_client_perform(client);

	if(err == ESP_OK){
		int status = esp_http_client_get_status_code(client);
		ESP_LOGI(TAG, "HTTP Status: %d", status);
		if(status == 200){
			char buffer[4096];
			int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
			if(len > 0){
				buffer[len] = 0;
			} else {
				ESP_LOGE(TAG, "HTTP read failed, len=%d", len);
				esp_http_client_cleanup(client);
				return ESP_FAIL;
			}
			
			cJSON *root = cJSON_Parse(buffer);
			if(root){
				cJSON *STATS = cJSON_GetObjectItem(root, "STATS");

				cJSON_Delete(root);
			}

		}
	}

	esp_http_client_cleanup(client);
	return err;
}
