#ifndef NTP_TIME_H
#define NTP_TIME_H

#include <stddef.h>

void init_ntp_time(void);
char *get_current_time_str(void);
bool is_time_synced(void);

void get_datetime_from_timestamp(int timestamp_v, char *buffer, size_t buffer_size);
char *get_date_from_timestamp(int timestamp);

#endif // NTP_TIME_H
