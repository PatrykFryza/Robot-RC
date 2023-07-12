#ifndef ROBOT_ADC_H
#define ROBOT_ADC_H


/** 
 * @brief Calculates joystick input to a parameter used for setting motor speed
 * 
 * @param value raw joystick input
 * 
 * @return data ready to send via ESP_NOW
 * 
 */
int8_t direction_calculator(int value);

/** 
 * @brief Initialize ADC pins
 * 
 */
void adc_init();

/** 
 * @brief Task that reads joystick and potentiometr input
 * 
 * @param pvParameter
 * 
 */
void adc_task(void *pvParameter);

#endif
