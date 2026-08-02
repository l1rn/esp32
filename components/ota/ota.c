#include "ota.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "common_types.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static const char *TAG = "OTA";

_i64 last_send_time = 0;
const _i64 INTERVAL_US = 2000000;

esp_err_t ota_post_handler(httpd_req_t *req){
	esp_ota_handle_t ota_handle;
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", 
			update_partition->subtype, update_partition->address);

	esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);

	if(err != ESP_OK){
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}

	char buf[1024];
	int recv;
	int remaining = req->content_len;

	while(remaining > 0){
		if((recv = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0){
			if(recv == HTTPD_SOCK_ERR_TIMEOUT) continue;
			esp_ota_abort(ota_handle);
			httpd_resp_send_500(req);
			return ESP_FAIL;
		}

		esp_ota_write(ota_handle, buf, recv);
		remaining -= recv;
		_i64 now = esp_timer_get_time();
		if((now) - last_send_time >= INTERVAL_US || remaining == 0){
			last_send_time = now;
			int bytes_received = req->content_len - remaining;
			int percent = (bytes_received * 100) / req->content_len;
			char out[40];
			int len = snprintf(out, sizeof(out), "[%d%%] %d/%d bytes\n", percent, bytes_received, req->content_len);

			httpd_resp_send_chunk(req, out, len);

		}
	}

	if(esp_ota_end(ota_handle) == ESP_OK && esp_ota_set_boot_partition(update_partition) == ESP_OK){
		httpd_resp_send_chunk(req, "\nOTA success! Rebooting...\n", HTTPD_RESP_USE_STRLEN);
		httpd_resp_send_chunk(req, NULL, 0);
		
		vTaskDelay(pdMS_TO_TICKS(1000));
		esp_restart();
	}

	httpd_resp_send_500(req);
	return ESP_FAIL;
}

static const httpd_uri_t ota_firmware = {
	.uri = "/update-firmware",
	.method = HTTP_POST,
	.handler = ota_post_handler,
};

esp_err_t register_ota_uri(httpd_handle_t server){
	if(server != NULL){
		httpd_register_uri_handler(server, &ota_firmware);
		return ESP_OK;
	}
	return ESP_FAIL;

	
}
