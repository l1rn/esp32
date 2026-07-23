#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

void app_configure(void);

void wifi_process(void);
void wifi_ntp_start(void);

void main_loop(void);
void project_cleanup(void);

#endif // PROCESS_MANAGER_H
