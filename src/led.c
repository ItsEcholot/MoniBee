#include <freertos/FreeRTOS.h>
#include <math.h>
#include <esp_log.h>
#include <driver/ledc.h>
#include <esp_pm.h>
#include <led_strip.h>

#include "led.h"

#define TAG "led"
#define LEDC_OUTPUT_IO 15
#define RGB_LED_IO 8
#define RGB_RMT_RES_HZ  (10 * 1000 * 1000) // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

static led_strip_handle_t led_strip;

void led_task(void *param)
{
  ESP_LOGI(TAG, "Starting task");
  init_led();
  set_led_percent(25);
  set_rgb_led(25, 0, 0);
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  // vTaskDelete(NULL);
}

void init_led(void)
{
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
      .duty_resolution = LEDC_TIMER_13_BIT,
      .freq_hz = 1000,
      .clk_cfg = LEDC_USE_RC_FAST_CLK,
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
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_ALLOW_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  led_strip_config_t strip_config = {
      .strip_gpio_num = RGB_LED_IO,
      .max_leds = 1,
      .led_model = LED_MODEL_WS2812,
      .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
      .flags = {
          .invert_out = false,
      }
  };
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = RGB_RMT_RES_HZ,
      .mem_block_symbols = 0,
      .flags = {
          .with_dma = 0,
      }
  };

  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}

void set_led_percent(uint32_t percent)
{
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pow(2, LEDC_TIMER_13_BIT) * (percent / 100.0)));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void set_rgb_led(uint8_t r, uint8_t g, uint8_t b)
{
  ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}