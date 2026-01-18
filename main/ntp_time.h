#ifndef NTP_TIME_H
#define NTP_TIME_H

void init_ntp_time(void);
char *get_current_time_str(void);
bool is_time_synced(void);

char *get_time_from_timestamp(int timestamp);
char *get_date_from_timestamp(int timestamp);

#endif // NTP_TIME_H
