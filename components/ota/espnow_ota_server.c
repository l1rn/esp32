#include "espnow_ota_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "esp_now.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ESP_NOW_OTA_SERVER";

static u8 target_mac[6];
static QueueHandle_t ack_queue = NULL;

typedef struct {
	u32 acked_chunk;
	bool status_ok;
} ack_response_t;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const u8 *data, int len){
	uint8_t *src_mac = (uint8_t *)recv_info->src_addr;
#else
static void espnow_recv_cb(const u8 *mac_addr, const u8 *data, int len){
	uint8_t *src_mac = (uint8_t *)mac_addr;
#endif
	if(len == sizeof(ota_packet_t)){
		ota_packet_t *pkt = (ota_packet_t *)data;
		if(pkt->type == OTA_CMD_ACK && ack_queue != NULL){
			ack_response_t resp = {
				.acked_chunk = pkt->chunk_idx,
				.status_ok = true
			};
			xQueueSendFromISR(ack_queue, &resp, NULL);
		}
	}
}

esp_err_t espnow_ota_server_init(const u8 *recv_mac){
	memcpy(target_mac, recv_mac, 6);

	ack_queue = xQueueCreate(5, sizeof(ack_response_t));
	if(ack_queue == NULL){
		ESP_LOGE(TAG, "Failed to create ACK channel");
		return ESP_FAIL;
	}

	ESP_ERROR_CHECK(esp_now_init());
	ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
	
	esp_now_peer_info_t peer = {};
	memcpy(peer.peer_addr, recv_mac, 6);
	peer.channel = CONFIG_ESP_PEER_CHANNEL;
	peer.encrypt = false;

	if(!esp_now_is_peer_exist(recv_mac))
		ESP_ERROR_CHECK(esp_now_add_peer(&peer));

	ESP_LOGI(TAG, "ESP-NOW OTA Server initialized for target: %02x:%02x:%02x:%02x:%02x:%02x", 
			recv_mac[0], recv_mac[1], recv_mac[2],
			recv_mac[3], recv_mac[4], recv_mac[5]);
	return ESP_OK;
}

esp_err_t espnow_ota_server_start_transfer(const u8 *firmware_bin, size_t bin_size){
	u32 total_chunks = (bin_size + OTA_CHUNK_SIZE - 1) / OTA_CHUNK_SIZE;
	ota_packet_t tx_pkt;

	ESP_LOGI(TAG, "Starting OTA transfer... Size: %u bytes (%lu chunks)", bin_size, total_chunks);

	for(u32 chunk = 0; chunk < total_chunks; chunk++){
		size_t offset = chunk * OTA_CHUNK_SIZE;
		size_t current_chunk_len = (bin_size - offset > OTA_CHUNK_SIZE) ? OTA_CHUNK_SIZE : (bin_size - offset);

		memset(&tx_pkt, 0, sizeof(ota_packet_t));

		tx_pkt.type = OTA_CMD_DATA;
		tx_pkt.chunk_idx = chunk;
		tx_pkt.total_chunks = total_chunks;
		tx_pkt.data_len = (u16) current_chunk_len;
		memcpy(tx_pkt.payload, firmware_bin + offset, current_chunk_len);

		tx_pkt.crc32 = esp_rom_crc32_le(0, tx_pkt.payload, current_chunk_len);

		bool chunk_sent = false;
		u8 retries = 0;
		while(!chunk_sent && retries < 5){
			xQueueReset(ack_queue);
			esp_err_t res = esp_now_send(target_mac, (u8 *)&tx_pkt, sizeof(ota_packet_t));
			if(res != ESP_OK){
				ESP_LOGE(TAG, "Send error on chunk %lu: %s", chunk, esp_err_to_name(res));
			}

			ack_response_t ack_resp;
			if(xQueueReceive(ack_queue, &ack_resp, pdMS_TO_TICKS(100)) == pdTRUE){
				if(ack_resp.acked_chunk == chunk && ack_resp.status_ok){
					chunk_sent = true;
				}
			}
			else{
				retries++;
				ESP_LOGW(TAG, "ACK timeout for chunk %lu. Retrying (%d/5)...", chunk, retries);
			}
		}

		if (!chunk_sent) {
		    ESP_LOGE(TAG, "Transfer failed at chunk %lu after 5 retries!", chunk);
		    return ESP_FAIL;
		}

		if (chunk % 50 == 0 || chunk == total_chunks - 1) {
		    ESP_LOGI(TAG, "Progress: %lu / %lu chunks (%.1f%%)",
			     chunk + 1, total_chunks, ((float)(chunk + 1) / total_chunks) * 100.0f);
		}
	}
	ESP_LOGI(TAG, "Firmware transfer completed successfully!");
	return ESP_OK;
}
