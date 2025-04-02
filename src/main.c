#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_pm.h>
#include <esp_private/esp_clk.h>
#include <esp_sleep.h>
#include <nvs_flash.h>
#include <esp_zigbee_core.h>

#include "led.h"
#include "zigbee.h"
#include "button_boot.h"
#include "internal_temperature.h"

static esp_err_t esp_power_save_init(void)
{
  esp_err_t rc = ESP_OK;
#ifdef CONFIG_PM_ENABLE
  int cur_cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
  esp_pm_config_t pm_config = {
      .max_freq_mhz = cur_cpu_freq_mhz,
      .min_freq_mhz = cur_cpu_freq_mhz,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
      .light_sleep_enable = true
#endif
  };
  rc = esp_pm_configure(&pm_config);
#endif
  return rc;
}

void app_main()
{
  esp_zb_platform_config_t config = {
      .radio_config = {
          .radio_mode = ZB_RADIO_MODE_NATIVE,
      },
      .host_config = {
          .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
      },
  };
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_power_save_init());
  ESP_ERROR_CHECK(esp_zb_platform_config(&config));

  xTaskCreate(zigbee_task, "zigbee_task", 4096, NULL, 5, NULL);
  xTaskCreate(led_task, "led_task", 2048, NULL, 1, NULL);
  xTaskCreate(button_boot_task, "button_boot_task", 2048, NULL, 2, NULL);
  xTaskCreate(internal_temperature_task, "internal_temperature_task", 2048, NULL, 2, NULL);
}