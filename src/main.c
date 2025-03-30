#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "led.h"
#include "zigbee.h"
#include "button_boot.h"

void app_main() {
  xTaskCreate(led_task, "led_task", 2048, NULL, 1, NULL);
  xTaskCreate(zigbee_task, "zigbee_task", 4096, NULL, 5, NULL);
  xTaskCreate(button_boot_task, "button_boot_task", 2048, NULL, 2, NULL);
}