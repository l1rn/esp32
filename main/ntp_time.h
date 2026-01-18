#ifndef NTP_TIME_H
#define NTP_TIME_H

void init_ntp_time(void);
char *get_current_time_str(void);
bool is_time_synced(void);

#endif // NTP_TIME_H
