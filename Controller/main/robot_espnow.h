#ifndef ROBOT_ESPNOW_H
#define ROBOT_ESPNOW_H

typedef struct data_t
{
    uint16_t potentiometer;
    int8_t joy_x;
    int8_t joy_y;
    uint8_t speed_button;
} data_t;


/**
  * @brief     Callback function upon sending data
  *
  * @param     mac_addr where it have to send data
  *
  */
void esp_now_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status);

/** 
 * @brief Initialize wifi
 * 
 */
void app_wifi_init();

/** 
 * @brief Initialize esp_now
 * 
 */
void app_esp_now_init();

/** 
 * @brief Send data via esp_now
 * 
 * @param pvParameter
 * 
 */
void send_task(void* pvParameter);

#endif