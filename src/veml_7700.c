#include <freertos/FreeRTOS.h>
#include <veml7700.h>
#include <esp_log.h>
#include <math.h>
#include <esp_pm.h>

#include "veml_7700.h"
#include "zigbee.h"

#define TAG "veml_7700"
#define I2C_ADDRESS 0x10
#define VEML7700_POLY_COEF_A (6.0135e-13)
#define VEML7700_POLY_COEF_B (-9.3924e-9)
#define VEML7700_POLY_COEF_C (8.1488e-5)
#define VEML7700_POLY_COEF_D (1.0023)

const float veml7700_resolution_map[13][4] = {
    {0.0672, 0.0336, 0.5376, 0.2688},
    {0.0336, 0.0168, 0.2688, 0.1344},
    {0.0168, 0.0084, 0.1344, 0.0672},
    {0.0084, 0.0042, 0.0672, 0.0336},
    {},
    {},
    {},
    {},
    {0.1344, 0.0672, 1.0752, 0.5376},
    {},
    {},
    {},
    {0.2688, 0.1344, 2.1504, 1.0752},
};

void veml_7700_task(void *param)
{
  ESP_LOGI(TAG, "Starting task");

  i2c_master_bus_handle_t bus_handle;
  ESP_ERROR_CHECK(i2c_master_get_bus_handle(I2C_NUM_0, &bus_handle));

  veml7700_config_t dev_config = {
      .i2c_address = I2C_VEML7700_DEV_ADDR,
      .i2c_clock_speed = I2C_VEML7700_DEV_CLK_SPD,
      .gain = VEML7700_GAIN_DIV_8,
      .integration_time = VEML7700_INTEGRATION_TIME_50MS,
      .persistence_protect = VEML7700_PERSISTENCE_PROTECTION_4,
      .irq_enabled = false,
      .power_disabled = true,
      .power_saving_enabled = false,
      .power_saving_mode = VEML7700_POWER_SAVING_MODE_1};

  veml7700_handle_t dev_handle;
  ESP_ERROR_CHECK(veml7700_init(bus_handle, &dev_config, &dev_handle));

  ESP_LOGI(TAG, "initialized");

  const float resolution = veml7700_resolution_map[dev_config.integration_time][dev_config.gain];

  esp_pm_lock_handle_t pm_lock;
  ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "veml7700", &pm_lock));
  while (true)
  {
    uint16_t counts;
    ESP_ERROR_CHECK(esp_pm_lock_acquire(pm_lock));
    ESP_ERROR_CHECK(veml7700_enable(dev_handle));
    vTaskDelay(3 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(veml7700_get_ambient_light_counts(dev_handle, &counts));
    ESP_ERROR_CHECK(veml7700_disable(dev_handle));
    ESP_ERROR_CHECK(esp_pm_lock_release(pm_lock));

    float lux = (float)counts * resolution;
    /* apply correction formula for illumination > 1000 lux */
    if (lux > 1000)
    {
      lux = (VEML7700_POLY_COEF_A * powf(lux, 4)) +
            (VEML7700_POLY_COEF_B * powf(lux, 3)) +
            (VEML7700_POLY_COEF_C * powf(lux, 2)) +
            (VEML7700_POLY_COEF_D * lux);
    }

    uint16_t value = (uint16_t)(10000 * log10(lux) + 1);
    ESP_LOGI(TAG, "%0.4f lux", lux);
    esp_zb_lock_acquire(1000 / portTICK_PERIOD_MS);
    esp_zb_zcl_set_attribute_val(
        ZIGBEE_VEML_7700_ENDPOINT_ID,
        ESP_ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_ID,
        &value,
        false);
    esp_zb_lock_release();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}