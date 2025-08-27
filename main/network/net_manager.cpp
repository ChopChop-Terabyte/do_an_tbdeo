#include "net_manager.hpp"

#include "core/event_manager.hpp"
// #include "esp_log.h"

using namespace core;

namespace network {
    auto &ev_net_manager = EventManager::instance();

    void NetManager::init() {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        wifi_ = std::make_unique<Wifi>();
    }

    void NetManager::start() {
        init();

        xTaskCreate([](void *arg) { static_cast<NetManager *>(arg)->start_task(arg); },
            "Start net manager task", 1024 * 2, this, 5, NULL
        );

        wifi_->start();
    }

    void NetManager::start_task(void *pvParameters) {
        ev_net_manager.subscribe(EventID::NET_STATUS, [this](void *data) { this->event_network_status(data); });
        vTaskDelete(NULL);
    }

    void NetManager::event_network_status(void *data) {
        is_connected_ = *static_cast<NetStatus *>(data);

        // if (is_connected_ == NetStatus::GOT_IP) ESP_LOGW("Network Status", "GOT IP");
        // else ESP_LOGE("Network Status", "NO IP");
    }

} // namespace network
