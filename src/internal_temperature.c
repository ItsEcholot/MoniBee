#include <esp_check.h>
#include <driver/temperature_sensor.h>
#include <freertos/FreeRTOS.h>
#include <esp_zigbee_core.h>
#include <ha/esp_zigbee_ha_standard.h>

#include "internal_temperature.h"
#include "zigbee.h"

#define TAG "internal_temperature"

const temperature_sensor_config_t temp_config = {
    .range_min = 18,
    .range_max = 45,
    .flags = {
        .allow_pd = true,
    },
};

void internal_temperature_task(void *param)
{
  int16_t last_value = 0;
  temperature_sensor_handle_t temp_handle = NULL;

  ESP_LOGI(TAG, "Starting task");
  while (true)
  {
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_config, &temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));

    float temp;
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &temp));
    int16_t value = (int16_t)(temp * 100);

    ESP_ERROR_CHECK(temperature_sensor_disable(temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_uninstall(temp_handle));

    ESP_LOGI(TAG, "%d.%02d °C", value / 100, value % 100);
    esp_zb_lock_acquire(1000 / portTICK_PERIOD_MS);
    esp_zb_zcl_set_attribute_val(
        ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID,
        ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
        &value,
        false);
    esp_zb_lock_release();

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}