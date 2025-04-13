#pragma once

#include "esp_zigbee_core.h"

// #define ZIGBEE_ROUTER_MODE
#define ZIGBEE_MANUFACTURER_NAME "\x07""Echolot"
#define ZIGBEE_MODEL_IDENTIFIER "\x07""MoniBee"
#define ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID 10
#define ZIGBEE_VEML_7700_ENDPOINT_ID 11
#define ZIGBEE_DDC_ENDPOINT_ID 12
#define ZIGBEE_DDC_CLUSTER_ID 0xFF00
#define ZIGBEE_DDC_INPUT_SELECT_ATTR_ID 0x0000
#define ZIGBEE_DDC_BRIGHTNESS_ATTR_ID 0x0001
#define ZIGBEE_DDC_POWER_MODE_ATTR_ID 0x0002
#define ZIGBEE_TX_POWER -15
#define ZIGBEE_KEEPALIVE 500

void zigbee_task(void *param);
void init_zigbee(void);
void setup_devices(void);