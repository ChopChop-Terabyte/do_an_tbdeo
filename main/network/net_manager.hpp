#pragma once

#include <memory>
#include "network/wifi/wifi.hpp"

namespace network {
    enum class NetStatus {
        NO_IP = 0,
        GOT_IP
    };

    class NetManager {
    public:
        void init();
        void start();

        void start_task(void *pvParameters);
        void event_network_status(void *data);

        bool is_connected() {
            if (is_connected_ == NetStatus::GOT_IP) return true;
            else return false;
        }

    private:
        std::unique_ptr<Wifi> wifi_;

        NetStatus is_connected_ = NetStatus::NO_IP;

    }; // class NetManager

} // namespace network
