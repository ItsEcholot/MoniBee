#include <esp_check.h>
#include <esp_mac.h>
#include <esp_zigbee_core.h>
#include <ha/esp_zigbee_ha_standard.h>
#include <esp_zigbee_type.h>

#include "zcl_utility.h"
#include "zigbee.h"
#include "internal_temperature.h"
#include "veml_7700.h"
#include "ddc.h"
#include "ir_led.h"
#include "ir_nec_encoder.h"

#define TAG "zigbee"

static TaskHandle_t temperature_task_handle = NULL;
static TaskHandle_t veml_7700_task_handle = NULL;
static TaskHandle_t ddc_task_handle = NULL;
static TaskHandle_t ir_led_task_handle = NULL;

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message);
static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message);
static esp_err_t zb_custom_cmd_handler(const esp_zb_zcl_custom_cluster_command_message_t *message);

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
  esp_zb_core_action_handler_register(zb_action_handler);

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
  esp_zb_basic_cluster_cfg_t basic_cfg = {
      .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
      .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
  };
  esp_zb_identify_cluster_cfg_t identify_cfg = {
      .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
  };

  esp_zb_temperature_sensor_cfg_t temp_sensor_cfg = ESP_ZB_DEFAULT_TEMPERATURE_SENSOR_CONFIG();
  temp_sensor_cfg.temp_meas_cfg.min_value = -1000;
  temp_sensor_cfg.temp_meas_cfg.max_value = 8000;
  ep_config = (esp_zb_endpoint_config_t){
      .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
      .app_device_version = 1,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .endpoint = ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID,
  };
  ESP_ERROR_CHECK(esp_zb_ep_list_add_ep(ep_list, esp_zb_temperature_sensor_clusters_create(&temp_sensor_cfg), ep_config));
  ESP_ERROR_CHECK(esp_zcl_utility_add_ep_basic_manufacturer_info(ep_list, ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID, &info));

  esp_zb_light_sensor_cfg_t light_sensor_cfg = {
      .basic_cfg = basic_cfg,
      .identify_cfg = identify_cfg,
      .illuminance_cfg = {
          .max_value = 30000,
          .min_value = 0,
          .measured_value = ESP_ZB_ZCL_VALUE_S16_NONE,
      }};
  ep_config = (esp_zb_endpoint_config_t){
      .app_device_id = ESP_ZB_HA_LIGHT_SENSOR_DEVICE_ID,
      .app_device_version = 1,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .endpoint = ZIGBEE_VEML_7700_ENDPOINT_ID,
  };
  esp_zb_cluster_list_t *light_clusters = esp_zb_light_sensor_clusters_create(&light_sensor_cfg);
  ESP_ERROR_CHECK(esp_zb_ep_list_add_ep(ep_list, light_clusters, ep_config));
  ESP_ERROR_CHECK(esp_zcl_utility_add_ep_basic_manufacturer_info(ep_list, ZIGBEE_VEML_7700_ENDPOINT_ID, &info));

  ep_config = (esp_zb_endpoint_config_t){
      .app_device_id = ESP_ZB_HA_CUSTOM_ATTR_DEVICE_ID,
      .app_device_version = 1,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .endpoint = ZIGBEE_DDC_ENDPOINT_ID,
  };
  esp_zb_cluster_list_t *ddc_clusters = esp_zb_zcl_cluster_list_create();
  esp_zb_attribute_list_t *ddc_attrs = esp_zb_zcl_attr_list_create(ZIGBEE_DDC_CLUSTER_ID);
  uint16_t uint16_placeholder_value = ESP_ZB_ZCL_VALUE_U16_NONE;
  uint8_t uint8_placeholder_value = ESP_ZB_ZCL_VALUE_U8_NONE;
  ESP_ERROR_CHECK(esp_zb_custom_cluster_add_custom_attr(
      ddc_attrs,
      ZIGBEE_DDC_INPUT_SELECT_ATTR_ID,
      ESP_ZB_ZCL_ATTR_TYPE_16BIT_ENUM,
      ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
      &uint16_placeholder_value));
  ESP_ERROR_CHECK(esp_zb_custom_cluster_add_custom_attr(
      ddc_attrs,
      ZIGBEE_DDC_BRIGHTNESS_ATTR_ID,
      ESP_ZB_ZCL_ATTR_TYPE_U16,
      ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
      &uint16_placeholder_value));
  ESP_ERROR_CHECK(esp_zb_custom_cluster_add_custom_attr(
      ddc_attrs,
      ZIGBEE_DDC_POWER_MODE_ATTR_ID,
      ESP_ZB_ZCL_ATTR_TYPE_8BIT_ENUM,
      ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
      &uint8_placeholder_value));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(ddc_clusters, esp_zb_basic_cluster_create(&basic_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(ddc_clusters, esp_zb_identify_cluster_create(&identify_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_custom_cluster(ddc_clusters, ddc_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_ep_list_add_ep(ep_list, ddc_clusters, ep_config));
  ESP_ERROR_CHECK(esp_zcl_utility_add_ep_basic_manufacturer_info(ep_list, ZIGBEE_DDC_ENDPOINT_ID, &info));

  ep_config = (esp_zb_endpoint_config_t){
      .app_device_id = ESP_ZB_HA_CUSTOM_ATTR_DEVICE_ID,
      .app_device_version = 1,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .endpoint = ZIGBEE_IR_ENDPOINT_ID,
  };
  esp_zb_cluster_list_t *ir_clusters = esp_zb_zcl_cluster_list_create();
  esp_zb_attribute_list_t *ir_attrs = esp_zb_zcl_attr_list_create(ZIGBEE_IR_CLUSTER_ID);
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(ir_clusters, esp_zb_basic_cluster_create(&basic_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(ir_clusters, esp_zb_identify_cluster_create(&identify_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_custom_cluster(ir_clusters, ir_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_ep_list_add_ep(ep_list, ir_clusters, ep_config));
  ESP_ERROR_CHECK(esp_zcl_utility_add_ep_basic_manufacturer_info(ep_list, ZIGBEE_IR_ENDPOINT_ID, &info));

  esp_zb_device_register(ep_list);
}

void deferred_start_tasks(void)
{
  if (!temperature_task_handle)
    xTaskCreate(internal_temperature_task, "internal_temperature_task", 2048, NULL, 1, &temperature_task_handle);
  if (!veml_7700_task_handle)
    xTaskCreate(veml_7700_task, "veml_7700_task", 2048, NULL, 2, &veml_7700_task_handle);
  if (!ddc_task_handle)
    xTaskCreate(ddc_task, "ddc_task", 2048, NULL, 3, &ddc_task_handle);
  if (!ir_led_task_handle)
    xTaskCreate(ir_led_task, "ir_led_task", 2048, NULL, 3, &ir_led_task_handle);
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

      deferred_start_tasks();

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

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
  esp_err_t ret = ESP_OK;
  switch (callback_id)
  {
  case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
    ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
    break;
  case ESP_ZB_CORE_CMD_CUSTOM_CLUSTER_REQ_CB_ID:
    ret = zb_custom_cmd_handler((esp_zb_zcl_custom_cluster_command_message_t *)message);
    break;
  case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID:
    break;
  default:
    ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
    break;
  }
  return ret;
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
  esp_err_t ret = ESP_OK;
  ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
  ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                      message->info.status);
  ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
           message->attribute.id, message->attribute.data.size);

  if (message->info.dst_endpoint == ZIGBEE_DDC_ENDPOINT_ID)
  {
    if (message->info.cluster == ZIGBEE_DDC_CLUSTER_ID)
    {
      ddc_command_t command = {0};
      switch (message->attribute.id)
      {
      case ZIGBEE_DDC_INPUT_SELECT_ATTR_ID:
        command.operation = DDC_VCP_OP_INPUT;
        command.value = *(uint16_t *)message->attribute.data.value;
        break;
      case ZIGBEE_DDC_BRIGHTNESS_ATTR_ID:
        command.operation = DDC_VCP_OP_BRIGHTNESS;
        command.value = *(uint16_t *)message->attribute.data.value;
        break;
      case ZIGBEE_DDC_POWER_MODE_ATTR_ID:
        command.operation = DDC_VCP_OP_POWER_MODE;
        command.value = *(uint8_t *)message->attribute.data.value;
        break;
      default:
        break;
      }
      xMessageBufferSend(ddc_message_buffer, &command, sizeof(command), 100 / portTICK_PERIOD_MS);
    }
  }
  return ret;
}

static esp_err_t zb_custom_cmd_handler(const esp_zb_zcl_custom_cluster_command_message_t *message)
{
  ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
  ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                      message->info.status);
  const uint8_t *payload = (const uint8_t *)message->data.value;

  switch (message->info.command.id)
  {
  case ZIGBEE_IR_COMMAND_ATTR_ID:
    if (message->data.size != 4)
      return ESP_ERR_INVALID_SIZE;

    ir_nec_scan_code_t nec_code = {
        .address = payload[0] | (payload[1] << 8),
        .command = payload[2] | (payload[3] << 8),
    };
    xMessageBufferSend(ir_led_message_buffer, &nec_code, sizeof(nec_code), 100 / portTICK_PERIOD_MS);
    break;
  default:
    ESP_LOGI(TAG, "Receive custom command: %d from address 0x%04hx", message->info.command.id, message->info.src_address.u.short_addr);
    ESP_LOGI(TAG, "Payload size: %d", message->data.size);
    ESP_LOG_BUFFER_CHAR(TAG, ((uint8_t *)message->data.value) + 1, message->data.size - 1);
  }

  return ESP_OK;
}