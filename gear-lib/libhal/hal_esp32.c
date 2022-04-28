
#include "libhal.h"
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>

#define TAG "esp32"

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    wifi_event_cb_t event_cb = (wifi_event_cb_t)ctx;
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        break;
    default:
        break;
    }
    event_cb((void *)event);
    return ESP_OK;
}

void wifi_setup(const char *ssid, const char *pswd, wifi_event_cb_t event_cb)
{
    wifi_config_t wifi_cfg;

    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ESP_ERROR_CHECK( nvs_flash_init() );
    }

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, event_cb));

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    memset(&wifi_cfg, 0, sizeof(wifi_config_t));
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid));
    strncpy((char *)wifi_cfg.sta.password, pswd, sizeof(wifi_cfg.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_setup finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
                    (char *)wifi_cfg.sta.ssid, (char *)wifi_cfg.sta.password);
}
