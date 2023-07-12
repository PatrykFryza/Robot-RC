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

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (100)

static QueueHandle_t recv_queue;
static const char* TAG = "esp_now";
static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t esp_mac[ESP_NOW_ETH_ALEN] = {0xC8, 0xC9, 0xA3, 0xCE, 0xF5, 0xD8}; // <== change to desired MAC
// static const uint8_t esp_mac[ESP_NOW_ETH_ALEN] = {0x58, 0xBF, 0x25, 0x91, 0xCC, 0x70}; // <== change to desired MAC

// struct for data to send
typedef struct data_t
{
    int value;
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
        printf("%d\n", recv_data.value);
        recv_data.value = map(recv_data.value, 0, 4095, 300, 2000);
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, recv_data.value));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        
    }

    vTaskDelete(NULL);
}


void app_main(void)
{
    // init gpio
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = 18,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

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