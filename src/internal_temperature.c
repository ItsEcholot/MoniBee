#include "esp_check.h"
#include <driver/temperature_sensor.h>
#include "freertos/FreeRTOS.h"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"

#include "internal_temperature.h"
#include "zigbee.h"

#define TAG "internal_temperature"

static temperature_sensor_handle_t temp_handle = NULL;
static temperature_sensor_config_t temp_config = {
    .range_min = -10,
    .range_max = 80,
};

void internal_temperature_task(void *param)
{
  ESP_LOGI(TAG, "Starting task");
  ESP_ERROR_CHECK(temperature_sensor_install(&temp_config, &temp_handle));
  while (true)
  {
    int16_t value = (int16_t)(get_internal_temperature() * 100);
    int16_t current_value = *(int16_t *)(esp_zb_zcl_get_attribute(
      ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID,
      ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID
    )->data_p);

    if (current_value / 100 == value / 100)
    {
      vTaskDelay(3000 / portTICK_PERIOD_MS);
      continue;
    }

    ESP_LOGI(TAG, "Updating internal temperature: %d.%02d", value / 100, value % 100);
    esp_zb_lock_acquire(1000 / portTICK_PERIOD_MS);
    esp_zb_zcl_set_attribute_val(
      ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID,
      ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
      ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
      &value,
      false
    );
    esp_zb_zcl_report_attr_cmd_t report_attr_cmd = {0};
    report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;
    report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    report_attr_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
    report_attr_cmd.zcl_basic_cmd.src_endpoint = ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID;
    esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
    esp_zb_lock_release();
  }
}

float get_internal_temperature(void)
{
  ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
  float temp;
  ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &temp));
  ESP_ERROR_CHECK(temperature_sensor_disable(temp_handle));
  return temp;
}