#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <driver/gpio.h>
#include "esp_zigbee_core.h"

#include "button_boot.h"

#define TAG "button_boot"

static void IRAM_ATTR button_isr_handler(void *arg)
{
  esp_zb_factory_reset();
}

void button_boot_task(void *param)
{
  ESP_LOGI(TAG, "Button boot task started");
  gpio_config_t button_config = {
    .pin_bit_mask = BIT64(GPIO_NUM_9),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE
  };
  gpio_config(&button_config);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_NUM_9, button_isr_handler, NULL);

  vTaskSuspend(NULL);
}