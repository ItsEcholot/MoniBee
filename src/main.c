#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_pm.h>
#include <esp_private/esp_clk.h>
#include <esp_sleep.h>
#include <nvs_flash.h>
#include <esp_zigbee_core.h>
#include <soc/usb_serial_jtag_reg.h>

#include "led.h"
#include "zigbee.h"
#include "button_boot.h"
#include "internal_temperature.h"

static bool has_usb_connection(void)
{
  uint32_t *frame_num_val = (uint32_t *)USB_SERIAL_JTAG_FRAM_NUM_REG;
  uint32_t first_read_val = *frame_num_val;
  vTaskDelay(50 / portTICK_PERIOD_MS);
  return *frame_num_val != first_read_val;
}

static esp_err_t esp_power_save_init(void)
{
  esp_err_t rc = ESP_OK;
#ifdef CONFIG_PM_ENABLE
  int cur_cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
  esp_pm_config_t pm_config = {
      .max_freq_mhz = cur_cpu_freq_mhz,
      .min_freq_mhz = cur_cpu_freq_mhz,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
      .light_sleep_enable = !has_usb_connection(),
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
}