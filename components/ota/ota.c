#include "ota.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "common_types.h"
#include "espnow_ota_server.h"

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

static esp_err_t ota_post_espnow_handler(httpd_req_t *req){
	size_t bin_size = req->content_len;
	if(bin_size == 0){
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Zero size firmware");
		return ESP_FAIL;
	}
	u8 *bin_buf = malloc(bin_size);
	if (!bin_buf) {
		ESP_LOGE(TAG, "Failed to allocate %d bytes for ESP-NOW OTA", bin_size);
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of Memory");
		return ESP_FAIL;
	}

	size_t total_read = 0;
	
	while(total_read < bin_size){
		int recv_len = httpd_req_recv(req, (char *)(bin_buf + total_read), bin_size - total_read);
		if(recv_len < 0){
			if(recv_len == HTTPD_SOCK_ERR_TIMEOUT){
				continue;
			}
			ESP_LOGE(TAG, "HTTP recv failed at %d/%d bytes", total_read, bin_size);
			free(bin_buf);
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "HTTP Recv Error");
			return ESP_FAIL;
		}
		total_read += recv_len;
	}
	ESP_LOGI(TAG, "Successfully received %d bytes via HTTP. Starting ESP-NOW transfer...", total_read);

	esp_err_t err = espnow_ota_server_start_transfer(bin_buf, bin_size);
	free(bin_buf);

	if(err == ESP_OK){
		httpd_resp_sendstr(req, "OTA Sent Successfully over ESP-NOW");
	} else {
		httpd_resp_send_500(req);
	}
	return ESP_OK;
}

static const httpd_uri_t ota_espnow_firmware = {
	.uri = "/ota-over-en",
	.method = HTTP_POST,
	.handler = ota_post_espnow_handler
};

esp_err_t register_ota_uri(httpd_handle_t server){
	if(server != NULL){
		httpd_register_uri_handler(server, &ota_firmware);
		httpd_register_uri_handler(server, &ota_espnow_firmware);
		return ESP_OK;
	}
	return ESP_FAIL;

	
}
