#pragma once

typedef enum {
  DISPLAYPORT_1 = 0x0f,
  DISPLAYPORT_2 = 0x10,
  HDMI_1 = 0x11,
  HDMI_2 = 0x12,
  THUNDERBOLT_1 = 0x19,
  THUNDERBOLT_2 = 0x1b,
} ddc_input_enum_t;

extern MessageBufferHandle_t ddc_input_message_buffer;

void ddc_task(void *param);