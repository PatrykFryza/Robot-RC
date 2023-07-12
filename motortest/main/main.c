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

#define ESP_NOW_WIFI_MODE WIFI_MODE_STA
#define ESP_NOW_WIFI_IF WIFI_IF_STA
#define ESP_NOW_PMK "pmk1234567890123"
#define ESP_NOW_LMK "lmk1234567890123"
#define RECV_QUEUE_LEN 10
#define ESP_NOW_MAX_DELAY 512

#define LED_PIN 22

static QueueHandle_t recv_queue;
static const char* TAG = "esp_now";
static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t esp_mac[ESP_NOW_ETH_ALEN] = {0x40, 0x22, 0xD8, 0x06, 0xFA, 0x0C}; // <== change to desired MAC
// static const uint8_t esp_mac[ESP_NOW_ETH_ALEN] = {0x58, 0xBF, 0x25, 0x91, 0xCC, 0x70}; // <== change to desired MAC

// struct for data to send
typedef struct data_t
{
    int value;
} data_t;


// send callback
static void esp_now_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    if (status != ESP_NOW_SEND_SUCCESS)
        ESP_LOGW(TAG, "Send status: fail");
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
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb));

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
void send_task(void* pvParameters)
{
    data_t send_data;

    while (1)
    {
        send_data.value = adc1_get_raw(ADC1_CHANNEL_4);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(esp_now_send(esp_mac, (const uint8_t*)&send_data, sizeof(send_data)));
    }

    vTaskDelete(NULL);
}


void app_main(void)
{
    // init gpio
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);

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

    xTaskCreate(send_task, "sending data", 2048, NULL, 1, NULL);
}
