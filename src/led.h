#pragma once

#include <stdint.h>

void led_task(void *param);
void set_led_percent(uint32_t percent);
void init_led(void);