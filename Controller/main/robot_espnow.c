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

#define ESP_NOW_WIFI_MODE WIFI_MODE_STA
#define ESP_NOW_WIFI_IF WIFI_IF_STA
#define ESP_NOW_PMK "pmk1234567890123"
#define ESP_NOW_LMK "lmk1234567890123"

static const char* TAG = "esp_now";
static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t esp_mac[ESP_NOW_ETH_ALEN] = {0x40, 0x22, 0xD8, 0x06, 0xFA, 0x0C}; // <== change to desired MAC

data_t send_data;

void esp_now_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status){
    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    if (status != ESP_NOW_SEND_SUCCESS)
        ESP_LOGW(TAG, "Send status: fail");
}


void app_wifi_init(){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESP_NOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
}


void app_esp_now_init(){
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

void send_task(void *pvParameter){

    while (1)
    {
        ESP_ERROR_CHECK(esp_now_send(esp_mac, (const uint8_t*)&send_data, sizeof(send_data)));

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}