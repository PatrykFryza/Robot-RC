#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "robot_espnow.h"
#include "robot_adc.h"

extern data_t send_data;

int8_t direction_calculator(int value){
    int8_t direction = 0;
    if(value == 0) direction = -4;
    else if(value > 0 && value <= 500) direction = -3;
    else if(value > 500 && value <= 1000) direction = -2;
    else if(value > 1000 && value <= 1500) direction = -1;
    else if(value > 1500 && value <= 2000) direction = 0;
    else if(value > 2000 && value <= 2500) direction = 1;
    else if(value > 2500 && value <= 3000) direction = 2;
    else if(value > 3000 && value < 4095) direction = 3;
    else if(value == 4095) direction = 4;
    return direction;
}

void adc_init(){
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
}

void adc_task(void *pvParameter){
    int valuex;
    int valuey;
    while(1)
    {
        vTaskDelay( 20 / portTICK_PERIOD_MS);
        valuex = adc1_get_raw(ADC1_CHANNEL_4);
        valuey = adc1_get_raw(ADC1_CHANNEL_7);
        send_data.joy_x = direction_calculator(valuex);
        send_data.joy_y = direction_calculator(valuey);
        send_data.potentiometer = adc1_get_raw(ADC1_CHANNEL_5);
    }
}