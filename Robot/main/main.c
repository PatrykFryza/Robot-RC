#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/mcpwm.h"
#include <string.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "driver/ledc.h"

#define ESP_NOW_WIFI_MODE WIFI_MODE_STA
#define ESP_NOW_WIFI_IF WIFI_IF_STA
#define ESP_NOW_PMK "pmk1234567890123"
#define ESP_NOW_LMK "lmk1234567890123"
#define RECV_QUEUE_LEN 10
#define ESP_NOW_MAX_DELAY 512

static QueueHandle_t recv_queue;
static const char* TAG = "esp_now";
static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t esp_mac[ESP_NOW_ETH_ALEN] = {0xC8, 0xC9, 0xA3, 0xCE, 0xF5, 0xD8}; // <== change to desired MAC
// static const uint8_t esp_mac[ESP_NOW_ETH_ALEN] = {0x58, 0xBF, 0x25, 0x91, 0xCC, 0x70}; // <== change to desired MAC

// struct for data to send
typedef struct data_t
{
    uint16_t potentiometer;
    int8_t joy_x;
    int8_t joy_y;
    uint8_t speed_button;
} data_t;

// receive callback
static void esp_now_recv_cb(const uint8_t* mac_addr, const uint8_t* data, int data_len)
{
    if (mac_addr == NULL || data == NULL || data_len <= 0)
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    data_t recv_data;
    memcpy(&recv_data, data, sizeof(recv_data));

    if (xQueueSend(recv_queue, &recv_data, ESP_NOW_MAX_DELAY) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send receive queue fail");
    }
}


// wifi initialization
static void app_wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESP_NOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
}


// esp now initialization
static void app_esp_now_init()
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_cb));

    // add peers
    esp_now_peer_info_t* peer = NULL;
    peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
    }

    // broadcast
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 0;
    peer->ifidx = ESP_NOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));

    // other esp
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 0;
    peer->ifidx = ESP_NOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, esp_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));

    free(peer);
    peer = NULL;
}


// task for sending data

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


// task for handling received data
void recv_task(void* pvParameters)
{
    data_t recv_data;

    while (1)
    {
        xQueueReceive(recv_queue, &recv_data, portMAX_DELAY);
        printf("Joy x: %d\n", recv_data.joy_x);
        printf("Joy y: %d\n", recv_data.joy_y);
        printf("Pot: %d\n", recv_data.potentiometer);
        printf("Speed: %d\n\n", recv_data.speed_button);
    }

    vTaskDelete(NULL);
}


void app_main(void)
{

    // init esp now
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    app_wifi_init();

    app_esp_now_init();

    // create queue for received data
    recv_queue = xQueueCreate(RECV_QUEUE_LEN, sizeof(data_t));
    if (recv_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create a queue");
        abort();
    }

    xTaskCreate(recv_task, "handling received data", 2048, NULL, 1, NULL);

}