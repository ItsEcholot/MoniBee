#pragma once

#define DDC_VCP_OP_BRIGHTNESS 0x10
#define DDC_VCP_OP_INPUT 0x60
#define DDC_VCP_OP_POWER_MODE 0xD6

typedef enum {
  DISPLAYPORT_1 = 0x0f,
  DISPLAYPORT_2 = 0x10,
  HDMI_1 = 0x11,
  HDMI_2 = 0x12,
  THUNDERBOLT_1 = 0x19,
  THUNDERBOLT_2 = 0x1b,
} ddc_input_enum_t;

typedef struct {
  uint8_t operation;
  uint16_t value;
} ddc_command_t;

extern MessageBufferHandle_t ddc_input_message_buffer;

void ddc_task(void *param);