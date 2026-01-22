#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

void gpio_init(void);
void wifi_process(void);
void display_process(void);
void wifi_ntp_start(void);

void main_loop(void);

#endif // PROCESS_MANAGER_H
