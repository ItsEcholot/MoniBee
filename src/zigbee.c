#include <esp_check.h>
#include <esp_mac.h>
#include <esp_zigbee_core.h>
#include <ha/esp_zigbee_ha_standard.h>

#include "zcl_utility.h"
#include "zigbee.h"
#include "internal_temperature.h"
#include "veml_7700.h"

#define TAG "zigbee"

static TaskHandle_t temperature_task_handle = NULL;
static TaskHandle_t veml_7700_task_handle = NULL;

void zigbee_task(void *param)
{
  ESP_LOGI(TAG, "Starting task");

  esp_zb_cfg_t zb_nwk_cfg = {
      .install_code_policy = 0,
#ifndef ZIGBEE_ROUTER_MODE
      .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
      .nwk_cfg.zed_cfg = {
          .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,
          .keep_alive = ZIGBEE_KEEPALIVE,
      }
#else
      .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,
      .nwk_cfg.zczr_cfg = {
          .max_children = 10,
      },
#endif
  };

  esp_zb_sleep_enable(true);
  esp_zb_init(&zb_nwk_cfg);
  esp_zb_set_tx_power(ZIGBEE_TX_POWER);

  setup_devices();

#ifdef ZIGBEE_ROUTER_MODE
  ESP_ERROR_CHECK(esp_zb_set_channel_mask(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK));
  ESP_ERROR_CHECK(esp_zb_set_secondary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK));
#endif
  ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK));
  ESP_ERROR_CHECK(esp_zb_start(false));
  esp_zb_stack_main_loop();
}

void setup_devices(void)
{

  zcl_basic_manufacturer_info_t info = {
      .manufacturer_name = ZIGBEE_MANUFACTURER_NAME,
      .model_identifier = ZIGBEE_MODEL_IDENTIFIER,
  };

  esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
  esp_zb_endpoint_config_t ep_config;

  esp_zb_temperature_sensor_cfg_t temp_sensor_cfg = ESP_ZB_DEFAULT_TEMPERATURE_SENSOR_CONFIG();
  temp_sensor_cfg.temp_meas_cfg.min_value = -1000;
  temp_sensor_cfg.temp_meas_cfg.max_value = 8000;
  ep_config = (esp_zb_endpoint_config_t){
      .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
      .app_device_version = 1,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .endpoint = ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID,
  };
  esp_zb_ep_list_add_ep(ep_list, esp_zb_temperature_sensor_clusters_create(&temp_sensor_cfg), ep_config);
  esp_zcl_utility_add_ep_basic_manufacturer_info(ep_list, ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID, &info);

  esp_zb_device_register(ep_list);
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
  ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
  uint32_t *p_sg_p = signal_struct->p_app_signal;
  esp_err_t err_status = signal_struct->esp_err_status;
  esp_zb_app_signal_type_t sig_type = *p_sg_p;
  switch (sig_type)
  {
  case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
    ESP_LOGI(TAG, "Initialize Zigbee stack");
    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
    break;
  case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
  case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
    if (err_status == ESP_OK)
    {
      ESP_LOGI(TAG, "Device started up in%s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : " non");

      if (!temperature_task_handle)
        xTaskCreate(internal_temperature_task, "internal_temperature_task", 2048, NULL, 2, &temperature_task_handle);
      if (!veml_7700_task_handle)
        xTaskCreate(veml_7700_task, "veml_7700_task", 2048, NULL, 2, &veml_7700_task_handle);

      if (esp_zb_bdb_is_factory_new())
      {
        ESP_LOGI(TAG, "Start network steering");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
      }
      else
      {
        ESP_LOGI(TAG, "Device rebooted");
      }
    }
    else
    {
      ESP_LOGW(TAG, "%s failed with status: %s, retrying", esp_zb_zdo_signal_to_string(sig_type),
               esp_err_to_name(err_status));
      esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                             ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
    }
    break;
  case ESP_ZB_BDB_SIGNAL_STEERING:
    if (err_status == ESP_OK)
    {
      esp_zb_ieee_addr_t extended_pan_id;
      esp_zb_get_extended_pan_id(extended_pan_id);
      ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
               extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
               extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
               esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());

      esp_zb_app_signal_t reboot_signal;
      uint32_t reboot_signal_type = ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT;
      reboot_signal.p_app_signal = &reboot_signal_type;
      reboot_signal.esp_err_status = ESP_OK;
      esp_zb_app_signal_handler(&reboot_signal);
    }
    else
    {
      ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
      esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
    }
    break;
  case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
    esp_zb_sleep_now();
    break;
  default:
    ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
             esp_err_to_name(err_status));
    break;
  }
}