#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <driver/rmt_tx.h>

#include "ir_nec_encoder.h"

#define TAG "ir_led"
#define IR_LED_ANODE_GPIO 7
#define IR_LED_CATHODE_GPIO 6
#define IR_RESOLUTION_HZ 1000000

#define NEC_LEADING_CODE_DURATION_0 9000
#define NEC_LEADING_CODE_DURATION_1 4500
#define NEC_PAYLOAD_ZERO_DURATION_0 560
#define NEC_PAYLOAD_ZERO_DURATION_1 560
#define NEC_PAYLOAD_ONE_DURATION_0 560
#define NEC_PAYLOAD_ONE_DURATION_1 1690
#define NEC_REPEAT_CODE_DURATION_0 9000
#define NEC_REPEAT_CODE_DURATION_1 2250

static uint8_t ir_led_message_buffer_storage[sizeof(ir_nec_scan_code_t) * 10];
StaticMessageBuffer_t ir_led_message_buffer_struct;
MessageBufferHandle_t ir_led_message_buffer;

void ir_led_task(void *param)
{
  ESP_LOGI(TAG, "Starting task");
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << IR_LED_CATHODE_GPIO) | (1ULL << IR_LED_ANODE_GPIO),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io_conf));
  ESP_ERROR_CHECK(gpio_set_level(IR_LED_CATHODE_GPIO, 0));
  ESP_ERROR_CHECK(gpio_set_drive_capability(IR_LED_ANODE_GPIO, GPIO_DRIVE_CAP_3));
  ESP_ERROR_CHECK(gpio_set_drive_capability(IR_LED_CATHODE_GPIO, GPIO_DRIVE_CAP_3));

  rmt_tx_channel_config_t tx_channel_cfg = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = IR_RESOLUTION_HZ,
      .mem_block_symbols = 64, // amount of RMT symbols that the channel can store at a time
      .trans_queue_depth = 4,  // number of transactions that allowed to pending in the background, this example won't queue multiple transactions, so queue depth > 1 is sufficient
      .gpio_num = IR_LED_ANODE_GPIO,
  };
  rmt_channel_handle_t tx_channel = NULL;
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));

  rmt_carrier_config_t carrier_cfg = {
      .duty_cycle = 0.33,
      .frequency_hz = 38000,
  };
  ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

  rmt_transmit_config_t transmit_config = {
      .loop_count = 1,
  };
  ir_nec_encoder_config_t nec_encoder_cfg = {
      .resolution = IR_RESOLUTION_HZ,
  };
  rmt_encoder_handle_t nec_encoder = NULL;
  ESP_ERROR_CHECK(rmt_new_ir_nec_encoder(&nec_encoder_cfg, &nec_encoder));

  ir_led_message_buffer = xMessageBufferCreateStatic(
    sizeof(ir_led_message_buffer_storage), 
    ir_led_message_buffer_storage, 
    &ir_led_message_buffer_struct);

  ir_nec_scan_code_t nec_code = {0};
  while (true)
  {
    xMessageBufferReceive(ir_led_message_buffer, &nec_code, sizeof(nec_code), portMAX_DELAY);
    ESP_ERROR_CHECK(rmt_enable(tx_channel));

    ESP_LOGI(TAG, "Sending address: 0x%04X, command: 0x%04X", nec_code.address, nec_code.command);
    ESP_ERROR_CHECK(rmt_transmit(tx_channel, nec_encoder, &nec_code, sizeof(nec_code), &transmit_config));

    ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_channel, 1000));
    ESP_ERROR_CHECK(rmt_disable(tx_channel));
    // toggle = !toggle;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}