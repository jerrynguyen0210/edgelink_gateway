#include "app/gateway_app.hpp"
#include "esp_log.h"

namespace {

constexpr char kLogTag[] = "main";
GatewayApp gateway_app;

}  // namespace

extern "C" void app_main(void)
{
    const esp_err_t result = gateway_app.start();
    if (result != ESP_OK) {
        ESP_LOGE(kLogTag, "Gateway startup failed: %s", esp_err_to_name(result));
    }
}
