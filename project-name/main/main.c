#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/mcpwm.h"

#define MOTOR_PWM_FREQ_HZ 25000
#define ESP_AMOTOR_ENABLE 25
#define ESP_AMOTORL_PWM 32
#define ESP_AMOTORR_PWM 33
#define ESP_BMOTOR_ENABLE 19
#define ESP_BMOTORL_PWM 5
#define ESP_BMOTORR_PWM 18


static mcpwm_config_t pwm_config = {
    .frequency = MOTOR_PWM_FREQ_HZ,
    .cmpr_a = 0,
    .cmpr_b = 0,
    .duty_mode = MCPWM_DUTY_MODE_0,
    .counter_mode = MCPWM_UP_COUNTER,
};

void app_main(void)
{  
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1<<ESP_AMOTOR_ENABLE) | (1<<ESP_BMOTOR_ENABLE));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    
    gpio_config(&io_conf);

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, ESP_AMOTORR_PWM);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, ESP_AMOTORL_PWM);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, ESP_BMOTORR_PWM);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, ESP_BMOTORL_PWM);

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);

    while(1){
        gpio_set_level(ESP_BMOTOR_ENABLE, 1);
        gpio_set_level(ESP_AMOTOR_ENABLE, 1);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 85.0f);
        mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, 85.0f);
    }
}