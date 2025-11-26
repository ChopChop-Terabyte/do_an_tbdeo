#pragma once

#include <string>
#include "mqtt_client.h"

namespace protocols {
    class MQTT {
    public:
        esp_mqtt_client_handle_t client_ = nullptr;
        static bool is_connected_;

        MQTT(std::string server_address, uint32_t port, esp_mqtt_transport_t transport);
        ~MQTT();

        void init();
        void start();

        void start_task(void *pvParameters);
        void subscribe_list();
        int publish(const char *topic, const char *data);
        void on_data(void *event_data);
        void info_pub();

    private:
        std::string server_address_;
        uint32_t port_;
        esp_mqtt_transport_t transport_;

    }; // class MQTT

} // namespace protocols
