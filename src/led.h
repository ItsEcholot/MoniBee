#include <stdint.h>

#ifndef LED_H
#define LED_H

void led_task(void *param);
void set_led_percent(uint32_t percent);
void init_led(void);

#endif