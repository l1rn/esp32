#include "ntp_time.h"
#include "esp_sntp.h"
#include "time.h"
#include "esp_log.h"

static bool time_synced = false;
static const char *TAG = "NTP_TIME";

static void time_sync_callback(struct timeval *tv){
	time_synced = true;
	ESP_LOGI(TAG, "Time synced with NTP Server");
}

void init_ntp_time(void){
	setenv("TZ", "UTC-5", 1);
	tzset();

	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_setservername(1, "ru.pool.ntp.org");
	esp_sntp_setservername(2, "time.google.com");

	sntp_set_time_sync_notification_cb(time_sync_callback);

	esp_sntp_init();
	ESP_LOGI(TAG, "NTP time server started");
}

char *get_current_time_str(void){
	static char buffer[9];
	
	if(!time_synced){
		strcpy(buffer, "--:--:--");
		return buffer;
	}

	time_t now;
	struct tm timeinfo;

	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
	return buffer;
}



char *get_time_from_timestamp(int timestamp_v){
	static char buffer[6];
	
	time_t timestamp = (time_t)timestamp_v;
	struct tm *local_tm = localtime(&timestamp);

	strftime(buffer, sizeof(buffer), "%H:%M", local_tm);
	return buffer;
}

char *get_date_from_timestamp(int timestamp_v){
	static char buffer[6];

	time_t timestamp = (time_t)timestamp_v;
	struct tm *local_tm = localtime(&timestamp);

	strftime(buffer, sizeof(buffer), "%d-%m", local_tm);
	return buffer;
}
	
bool is_time_synced(void){
	return time_synced;
}
