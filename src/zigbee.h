#ifndef ZIGBEE_H
#define ZIGBEE_H

#include "esp_zigbee_core.h"

// #define ZIGBEE_ROUTER_MODE
#define ZIGBEE_MANUFACTURER_NAME "\x07""Echolot"
#define ZIGBEE_MODEL_IDENTIFIER "\x07""MoniBee"
#define ZIGBEE_INTERNAL_TEMPERATURE_ENDPOINT_ID 10
#define ZIGBEE_TX_POWER -15

void zigbee_task(void *param);
void init_zigbee(void);
void setup_devices(void);

typedef struct zcl_basic_manufacturer_info_s {
  char *manufacturer_name;
  char *model_identifier;
} zcl_basic_manufacturer_info_t;

/**
 * @brief Adds manufacturer information to the ZCL basic cluster of endpoint
 * 
 * @param[in] ep_list The pointer to the endpoint list with @p endpoint_id
 * @param[in] endpoint_id The endpoint identifier indicating where the ZCL basic cluster resides
 * @param[in] info The pointer to the basic manufacturer information
 * @return
 *      - ESP_OK: On success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 */
esp_err_t esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_ep_list_t *ep_list, uint8_t endpoint_id, zcl_basic_manufacturer_info_t *info);


#endif