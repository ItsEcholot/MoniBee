#include <freertos/FreeRTOS.h>
#include <math.h>
#include "esp_log.h"
#include "driver/ledc.h"
#include "led.h"

#define TAG "led"
#define LEDC_OUTPUT_IO 15

void led_task(void *param)
{
  ESP_LOGI(TAG, "Starting task");
  init_led();
  int percent = 0;
  while (true)
  {
    for (percent = 0; percent <= 100; percent += 10)
    {
      set_led_percent(percent);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    for (percent = 100; percent >= 0; percent -= 10)
    {
      set_led_percent(percent);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

void set_led_percent(uint32_t percent)
{
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pow(2, LEDC_TIMER_13_BIT) * (percent / 100.0)));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void init_led(void)
{
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
      .duty_resolution = LEDC_TIMER_13_BIT,
      .freq_hz = 4000,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = LEDC_OUTPUT_IO,
      .duty = 0,
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}