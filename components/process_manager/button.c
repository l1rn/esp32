#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button.h"

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO GPIO_NUM_2

void button_is_pressed(void){
	gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    bool last_button_state = 1; 

    while (1) {
        bool current_button_state = gpio_get_level(BUTTON_GPIO);

        if (current_button_state == 0 && last_button_state == 1) {
            printf("Button pressed!\n");
            gpio_set_level(LED_GPIO, !gpio_get_level(LED_GPIO));
        }

        last_button_state = current_button_state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void scan_button(void){}
