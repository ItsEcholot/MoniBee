#pragma once

#include <stdint.h>

void led_task(void *param);
void set_led_percent(uint32_t percent);
void set_rgb_led(uint8_t r, uint8_t g, uint8_t b);
void init_led(void);