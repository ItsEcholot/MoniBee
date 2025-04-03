#ifndef ZIGBEE_H
#define ZIGBEE_H

#include "esp_zigbee_core.h"

// #define ZIGBEE_ROUTER_MODE
#define ZIGBEE_MANUFACTURER_NAME "\x07""Echolot"
#define ZIGBEE_MODEL_IDENTIFIER "\x07""MoniBee"
#define ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID 10
#define ZIGBEE_TX_POWER -15
#define ZIGBEE_KEEPALIVE 500

void zigbee_task(void *param);
void init_zigbee(void);
void setup_devices(void);

#endif