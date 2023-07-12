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
#include "digital_read.h"
#include "freertos/portmacro.h"

extern data_t send_data;

void interrupt_gpio_init(){
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = ((1<<19) | (1<<26));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    
    gpio_config(&io_conf);

    send_data.speed_button = 1;
}

void interrupt_init(){
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(26, interrupt_function_slow, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(19, interrupt_function_fast, NULL));
}

void interrupt_function_slow(){
    //if(send_data.speed_button > 1) --(send_data.speed_button);
    printf("Cebula\n");
}

void interrupt_function_fast(){
    //if(send_data.speed_button < 5) ++(send_data.speed_button);
    printf("Cebula2\n");
}