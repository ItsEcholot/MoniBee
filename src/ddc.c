#include <freertos/FreeRTOS.h>
#include <driver/i2c_master.h>
#include <esp_log.h>

#include "ddc.h"
#include "zigbee.h"

#define TAG "ddc"
#define I2C_ADDRESS 0x37
#define VCP_OP_BRIGHTNESS 0x10
#define VCP_OP_INPUT 0x60

static uint8_t ddc_input_message_buffer_storage[8];
StaticMessageBuffer_t ddc_input_message_buffer_struct;
MessageBufferHandle_t ddc_input_message_buffer;

uint16_t read_vcp(i2c_master_dev_handle_t dev_handle, uint8_t operation, uint16_t fallback_value)
{
  uint8_t data[] = {
      0x51,
      0x82,
      0x01,
      operation,
      0,
  };
  data[4] = (I2C_ADDRESS << 1) ^ data[0] ^ data[1] ^ data[2] ^ data[3];
  uint8_t response[12] = {0};

  ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data, sizeof(data), 500));
  vTaskDelay(40 / portTICK_PERIOD_MS);
  ESP_ERROR_CHECK(i2c_master_receive(dev_handle, response, sizeof(response), 500));
  i2c_master_transmit_receive(dev_handle, data, sizeof(data), response, sizeof(response), 500);

  if (response[3] != 0x00)
  {
    return fallback_value;
  };
  if (response[4] != operation)
  {
    return fallback_value;
  }
  vTaskDelay(50 / portTICK_PERIOD_MS);
  return (response[8] << 8) | response[9];
}

void write_vcp(i2c_master_dev_handle_t dev_handle, uint8_t operation, uint16_t value)
{
  uint8_t data[] = {
      0x51,
      0x84,
      0x03,
      operation,
      value >> 8,
      value & 255,
      0,
  };
  data[6] = (I2C_ADDRESS << 1) ^ data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4] ^ data[5];

  ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data, sizeof(data), 500));
  vTaskDelay(50 / portTICK_PERIOD_MS);
}

void ddc_task(void *param)
{
  i2c_master_bus_handle_t bus_handle;
  ESP_ERROR_CHECK(i2c_master_get_bus_handle(I2C_NUM_0, &bus_handle));
  ESP_ERROR_CHECK(i2c_master_probe(bus_handle, I2C_ADDRESS, 500));

  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = I2C_ADDRESS,
      .scl_speed_hz = UINT32_C(100000),
  };
  i2c_master_dev_handle_t dev_handle;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

  ddc_input_message_buffer = xMessageBufferCreateStatic(
      sizeof(ddc_input_message_buffer_storage),
      ddc_input_message_buffer_storage,
      &ddc_input_message_buffer_struct);

  uint16_t last_brightness = 0;
  uint16_t last_input = 0;
  while (true)
  {
    uint16_t brightness = read_vcp(dev_handle, VCP_OP_BRIGHTNESS, last_brightness);
    uint16_t input = read_vcp(dev_handle, VCP_OP_INPUT, last_input);
    
    if (input != last_input)
    {
      ESP_LOGI(TAG, "Input: %d", input);
      last_input = input;
      esp_zb_lock_acquire(1000 / portTICK_PERIOD_MS);
      esp_zb_zcl_set_attribute_val(
          ZIGBEE_DDC_ENDPOINT_ID,
          ZIGBEE_DDC_CLUSTER_ID,
          ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
          ZIGBEE_DDC_INPUT_SELECT_ATTR_ID,
          &input,
          false);
      esp_zb_zcl_report_attr_cmd_t report_attr_cmd = {
          .zcl_basic_cmd = {
              .dst_addr_u = {
                  .addr_short = 0x0000,
              },
              .dst_endpoint = ZIGBEE_DDC_ENDPOINT_ID,
              .src_endpoint = ZIGBEE_DDC_ENDPOINT_ID,
          },
          .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          .clusterID = ZIGBEE_DDC_CLUSTER_ID,
          .attributeID = ZIGBEE_DDC_INPUT_SELECT_ATTR_ID,
          .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
      };
      ESP_ERROR_CHECK(esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd));
      esp_zb_lock_release();
    }

    if (brightness != last_brightness)
    {
      ESP_LOGI(TAG, "Brightness: %d", brightness);
      last_brightness = brightness;
      esp_zb_lock_acquire(1000 / portTICK_PERIOD_MS);
      esp_zb_zcl_set_attribute_val(
          ZIGBEE_DDC_ENDPOINT_ID,
          ZIGBEE_DDC_CLUSTER_ID,
          ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
          ZIGBEE_DDC_BRIGHTNESS_ATTR_ID,
          &brightness,
          false);
      esp_zb_zcl_report_attr_cmd_t report_attr_cmd = {
          .zcl_basic_cmd = {
              .dst_addr_u = {
                  .addr_short = 0x0000,
              },
              .dst_endpoint = ZIGBEE_DDC_ENDPOINT_ID,
              .src_endpoint = ZIGBEE_DDC_ENDPOINT_ID,
          },
          .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          .clusterID = ZIGBEE_DDC_CLUSTER_ID,
          .attributeID = ZIGBEE_DDC_BRIGHTNESS_ATTR_ID,
          .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
      };
      ESP_ERROR_CHECK(esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd));
      esp_zb_lock_release();
    }

    uint16_t next_input = 0;
    xMessageBufferReceive(ddc_input_message_buffer, &next_input, sizeof(next_input), 1000 / portTICK_PERIOD_MS);

    if (next_input != 0)
    {
      ESP_LOGI(TAG, "Setting input to %d", next_input);
      write_vcp(dev_handle, 0x60, next_input);
    }
  }
}