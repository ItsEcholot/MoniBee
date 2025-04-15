#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_zigbee_core.h>
#include <esp_sleep.h>

#include "button_boot.h"
#include "led.h"

#define TAG "button_boot"
#define BUTTON_IO GPIO_NUM_9

TaskHandle_t button_boot_task_handle = NULL;

static void IRAM_ATTR button_isr_handler(void *arg)
{
  xTaskResumeFromISR(button_boot_task_handle);
}

void button_boot_task(void *param)
{
  button_boot_task_handle = xTaskGetCurrentTaskHandle();
  ESP_LOGI(TAG, "Starting task");
  
  gpio_config_t button_config = {
    .pin_bit_mask = BIT64(BUTTON_IO),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE
  };
  ESP_ERROR_CHECK(gpio_wakeup_enable(BUTTON_IO, GPIO_INTR_LOW_LEVEL));
  ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
  ESP_ERROR_CHECK(gpio_config(&button_config));
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_IO, button_isr_handler, NULL));

  vTaskSuspend(NULL);
  set_rgb_led(0, 50, 0);
  esp_zb_factory_reset();
}