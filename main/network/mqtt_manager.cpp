#include "network/mqtt_manager.hpp"

#include "esp_log.h"

namespace {

constexpr char kLogTag[] = "mqtt";

}  // namespace

esp_err_t MqttManager::start(const char *broker_uri,
                             const char *username,
                             const char *password,
                             const char *topic)
{
    if (broker_uri == nullptr || broker_uri[0] == '\0' || topic == nullptr ||
        topic[0] == '\0') {
        ESP_LOGE(kLogTag, "MQTT broker URI or topic is not configured");
        return ESP_ERR_INVALID_ARG;
    }

    esp_mqtt_client_config_t config{};
    config.broker.address.uri = broker_uri;

    if (username != nullptr && username[0] != '\0') {
        config.credentials.username = username;
    }
    if (password != nullptr && password[0] != '\0') {
        config.credentials.authentication.password = password;
    }

    client_ = esp_mqtt_client_init(&config);
    if (client_ == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    topic_ = topic;
    esp_err_t result = esp_mqtt_client_register_event(
        client_, MQTT_EVENT_ANY, event_handler, this);
    if (result != ESP_OK) {
        esp_mqtt_client_destroy(client_);
        client_ = nullptr;
        return result;
    }

    result = esp_mqtt_client_start(client_);
    if (result != ESP_OK) {
        esp_mqtt_client_destroy(client_);
        client_ = nullptr;
        return result;
    }

    ESP_LOGI(kLogTag, "Connecting to broker: %s", broker_uri);
    return ESP_OK;
}

bool MqttManager::publish(const char *payload) const
{
    if (!connected_.load() || client_ == nullptr || topic_ == nullptr ||
        payload == nullptr) {
        return false;
    }

    const int message_id =
        esp_mqtt_client_enqueue(client_, topic_, payload, 0, 1, 0, true);
    if (message_id < 0) {
        ESP_LOGE(kLogTag, "Failed to enqueue MQTT message");
        return false;
    }

    ESP_LOGI(kLogTag, "Queued message id=%d topic=%s", message_id, topic_);
    return true;
}

bool MqttManager::is_connected() const
{
    return connected_.load();
}

void MqttManager::event_handler(void *context,
                                esp_event_base_t,
                                int32_t event_id,
                                void *event_data)
{
    auto *manager = static_cast<MqttManager *>(context);
    const auto *event = static_cast<const esp_mqtt_event_t *>(event_data);

    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            manager->connected_.store(true);
            ESP_LOGI(kLogTag, "Connected to MQTT broker");
            break;
        case MQTT_EVENT_DISCONNECTED:
            manager->connected_.store(false);
            ESP_LOGW(kLogTag, "Disconnected from MQTT broker");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(kLogTag, "Published message id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            manager->connected_.store(false);
            ESP_LOGE(kLogTag, "MQTT transport error");
            break;
        default:
            break;
    }
}
