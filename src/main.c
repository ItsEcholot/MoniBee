#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_pm.h>

#include "led.h"
#include "zigbee.h"
#include "button_boot.h"

void app_main() {
  esp_pm_config_t pm_config = {
    .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
    .min_freq_mhz = 80,
    .light_sleep_enable = false,
  };
  ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

  xTaskCreate(led_task, "led_task", 2048, NULL, 1, NULL);
  xTaskCreate(zigbee_task, "zigbee_task", 4096, NULL, 5, NULL);
  xTaskCreate(button_boot_task, "button_boot_task", 2048, NULL, 2, NULL);
}