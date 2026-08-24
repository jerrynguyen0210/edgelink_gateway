#include "network/wifi_manager.hpp"

#include <cstring>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

namespace {

constexpr char kLogTag[] = "wifi";
constexpr EventBits_t kConnectedBit = BIT0;
constexpr EventBits_t kFailedBit = BIT1;

esp_err_t initialize_nvs()
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result;
}

}  // namespace

esp_err_t WifiManager::start(const char *ssid, const char *password)
{
    if (ssid == nullptr || ssid[0] == '\0' || password == nullptr) {
        ESP_LOGE(kLogTag, "Wi-Fi SSID is not configured");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(initialize_nvs(), kLogTag, "Failed to initialize NVS");
    ESP_RETURN_ON_ERROR(esp_netif_init(), kLogTag, "Failed to initialize TCP/IP");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(),
                        kLogTag,
                        "Failed to create event loop");

    event_group_ = xEventGroupCreate();
    if (event_group_ == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    if (esp_netif_create_default_wifi_sta() == nullptr) {
        return ESP_FAIL;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config),
                        kLogTag,
                        "Failed to initialize Wi-Fi");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, this),
        kLogTag,
        "Failed to register Wi-Fi event handler");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, this),
        kLogTag,
        "Failed to register IP event handler");

    wifi_config_t wifi_config{};
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid),
                 ssid,
                 sizeof(wifi_config.sta.ssid) - 1U);
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.password),
                 password,
                 sizeof(wifi_config.sta.password) - 1U);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA),
                        kLogTag,
                        "Failed to set station mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config),
                        kLogTag,
                        "Failed to configure station");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), kLogTag, "Failed to start Wi-Fi");

    ESP_LOGI(kLogTag, "Connecting to SSID: %s", ssid);
    return ESP_OK;
}

bool WifiManager::wait_for_connection(TickType_t timeout) const
{
    if (event_group_ == nullptr) {
        return false;
    }

    const EventBits_t bits = xEventGroupWaitBits(
        event_group_, kConnectedBit | kFailedBit, pdFALSE, pdFALSE, timeout);
    return (bits & kConnectedBit) != 0;
}

bool WifiManager::is_connected() const
{
    return event_group_ != nullptr &&
           (xEventGroupGetBits(event_group_) & kConnectedBit) != 0;
}

void WifiManager::event_handler(void *context,
                                esp_event_base_t event_base,
                                int32_t event_id,
                                void *event_data)
{
    auto *manager = static_cast<WifiManager *>(context);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(manager->event_group_, kConnectedBit);

        if (manager->retry_count_ < CONFIG_EDGELINK_WIFI_MAX_RETRY) {
            ++manager->retry_count_;
            ESP_LOGW(kLogTag,
                     "Disconnected; retrying (%d/%d)",
                     manager->retry_count_,
                     CONFIG_EDGELINK_WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(kLogTag, "Unable to connect to the configured access point");
            xEventGroupSetBits(manager->event_group_, kFailedBit);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const auto *event = static_cast<const ip_event_got_ip_t *>(event_data);
        manager->retry_count_ = 0;
        xEventGroupClearBits(manager->event_group_, kFailedBit);
        xEventGroupSetBits(manager->event_group_, kConnectedBit);
        ESP_LOGI(kLogTag, "Connected, IP address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}
