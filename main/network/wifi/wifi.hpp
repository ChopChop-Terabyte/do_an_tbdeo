#pragma once

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"

#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1
#define ESPTOUCH_START_BIT      BIT2
#define ESPTOUCH_DONE_BIT       BIT3
#define DHCPS_OFFER_DNS         0x02

namespace network {
    enum class WifiMode {
        AP_MODE = 0,
        STA_MODE,
        APSTA_MODE,
        SMART_CONFIG_MODE
    };

    class Wifi {
    public:
        Wifi();
        ~Wifi();

        void init();
        void start();

        void start_task(void *pvParameters);
        void select_mode(WifiMode wifi_mode);
        static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
        void start_smartconfig();
        void stop_smartconfig();
        void event_smartconfig_toggle(void *data);

    private:
        esp_netif_t *esp_netif_sta_ = nullptr;
        esp_netif_t *esp_netif_ap_ = nullptr;

    }; // class Wifi

} // namespace network