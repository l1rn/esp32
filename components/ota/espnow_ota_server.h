#ifndef ESPNOW_OTA_SERVER_H
#define ESPNOW_OTA_SERVER_H

#include "esp_err.h"

#define OTA_CHUNK_SIZE 1024

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint16_t u16;
typedef enum {
	OTA_CMD_START = 1,
	OTA_CMD_DATA = 2,
	OTA_CMD_END = 3,
	OTA_CMD_ACK = 4
} ota_cmd_type_t;

typedef struct __attribute__((packed)) {
	u8 type;
	u32 chunk_idx;
	u32 total_chunks;
	u16 data_len;
	u32 crc32;
	u8 payload[OTA_CHUNK_SIZE];
} ota_packet_t;

esp_err_t espnow_ota_server_init(const u8 *receiver_mac);
esp_err_t espnow_ota_server_start_transfer(const u8 *firmware_bin, size_t bin_size);

#endif // ESPNOW_OTA_SERVER_H
