#ifndef DIGITAL_READ_H
#define DIGITAL_READ_H


/** 
 * @brief Initialize gpio of buttons with interrupts
 * 
 */
void interrupt_gpio_init();

/** 
 * @brief Install interrupt and assign functions to the buttons
 * 
 */
void interrupt_init();

/** 
 * @brief Interrupt function that makes the robot slower
 * 
 */
void interrupt_function_slow();

/** 
 * @brief Interrupt function that makes the robot faster
 * 
 */
void interrupt_function_fast();

#endif