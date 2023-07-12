#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "robot_espnow.h"
#include "robot_adc.h"
#include "digital_read.h"

void app_main(void){

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    app_wifi_init();

    app_esp_now_init();

    adc_init();

    interrupt_gpio_init();

    interrupt_init();

    xTaskCreate(&send_task, "sending data", 2048, NULL, 5, NULL);

    xTaskCreate(&adc_task, "read adc input", 2048, NULL, 5, NULL);

}