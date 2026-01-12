#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "mbedtls/md.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "ANTMINER";
#define BUFFER_SIZE 1024
#define REQ_BUFFER_SIZE 1024

// Compute MD5 hash as hex
static void md5_hex(const char *input, char *output) {
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

// Extract a value from WWW-Authenticate header
static void extract_value(const char *header, const char *key, char *out, size_t out_len) {
    char *start = strstr(header, key);
    if (!start) {
        out[0] = '\0';
        return;
    }
    start += strlen(key) + 2; // skip key and =" 
    char *end = strchr(start, '"');
    if (!end) {
        out[0] = '\0';
        return;
    }
    size_t len = end - start;
    if (len >= out_len) len = out_len - 1;
    strncpy(out, start, len);
    out[len] = '\0';
}

// Compute Digest header
static void build_digest_header(
    const char *username,
    const char *password,
    const char *realm,
    const char *nonce,
    const char *uri,
    const char *method,
    const char *cnonce,
    int nc,
    char *out_header,
    size_t out_len
) {
    char HA1[33], HA2[33], response[33], nc_str[9], tmp[256];
    snprintf(tmp, sizeof(tmp), "%s:%s:%s", username, realm, password);
    md5_hex(tmp, HA1);
    snprintf(tmp, sizeof(tmp), "%s:%s", method, uri);
    md5_hex(tmp, HA2);

    snprintf(nc_str, sizeof(nc_str), "%08x", nc);
    snprintf(tmp, sizeof(tmp), "%s:%s:%s:%s:auth:%s", HA1, nonce, nc_str, cnonce, HA2);
    md5_hex(tmp, response);

    snprintf(out_header, out_len,
             "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", qop=auth, nc=%s, cnonce=\"%s\", response=\"%s\"\r\n",
             username, realm, nonce, uri, nc_str, cnonce, response);
}

void antminer_get_data(void) {
    const char *host = "192.168.1.129";
    const int port = 80;
    const char *uri = "/cgi-bin/stats.cgi";
    const char *username = "root";
    const char *password = "root";

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr.s_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        ESP_LOGE(TAG, "Connection failed");
        close(sock);
        return;
    }

    // Step 1: Send initial request without Authorization
    char req[REQ_BUFFER_SIZE];
    snprintf(req, sizeof(req),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             uri, host);

    send(sock, req, strlen(req), 0);

    char buf[BUFFER_SIZE];
    int len = recv(sock, buf, sizeof(buf)-1, 0);
    if (len <= 0) {
        ESP_LOGE(TAG, "No response");
        close(sock);
        return;
    }
    buf[len] = '\0';

    // Step 2: Extract realm and nonce
    char realm[128], nonce[128];
    extract_value(buf, "realm", realm, sizeof(realm));
    extract_value(buf, "nonce", nonce, sizeof(nonce));
    ESP_LOGI(TAG, "realm=%s nonce=%s", realm, nonce);

    close(sock);

    // Step 3: Reconnect for authenticated request
    sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    char cnonce[] = "abcdef1234567890"; // can be random
    char auth_header[512];
    build_digest_header(username, password, realm, nonce, uri, "GET", cnonce, 1, auth_header, sizeof(auth_header));

    char line[256];

    // Request line
    snprintf(line, sizeof(line), "GET %s HTTP/1.1\r\n", uri);
    send(sock, line, strlen(line), 0);

    // Host header
    snprintf(line, sizeof(line), "Host: %s\r\n", host);
    send(sock, line, strlen(line), 0);

    // Connection header
    send(sock, "Connection: close\r\n", strlen("Connection: close\r\n"), 0);

    // Authorization header
    send(sock, auth_header, strlen(auth_header), 0);

    // End of headers
    send(sock, "\r\n", 2, 0);

    send(sock, req, strlen(req), 0);
    len = recv(sock, buf, sizeof(buf)-1, 0);
    if (len > 0) {
        buf[len] = '\0';
        ESP_LOGI(TAG, "Response:\n%s", buf);
    } else {
        ESP_LOGE(TAG, "No response on authenticated request");
    }

    close(sock);
}

