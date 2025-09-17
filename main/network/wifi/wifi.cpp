#include "wifi.hpp"
#include <cstring>
#include <memory>
#include "esp_mac.h"
#include "esp_log.h"

#include "common/config.h"
#include "core/event_manager.hpp"
#include "peripherals/gpio.hpp"
#include "network/net_manager.hpp"

static const char *TAG = "WIFI";
static const char *TAG_STA = "WIFI STA mode";
static const char *TAG_AP = "WIFI AP mode";
static const char *TAG_APSTA = "WIFI AP + STA mode";
static const char *TAG_SMART = "WIFI SMART mode";

using namespace core;
using namespace peripherals;

namespace network {
    auto &ev_wifi = EventManager::instance();
    void _button_publisher(LedLevel level);
    void _net_status_publisher(NetStatus net);
    LedLevel mess_led;
    NetStatus mess_net;

    static EventGroupHandle_t s_wifi_event_group = nullptr;
    static bool wifi_start = false;
    static bool smart_conf_start = false;
    static bool smart_conf_toggle = false;

    Wifi::Wifi() {}

    Wifi::~Wifi() {
        esp_smartconfig_stop();
        if (esp_netif_ap_) esp_netif_destroy(esp_netif_ap_);
        if (esp_netif_sta_) esp_netif_destroy(esp_netif_sta_);
        if (wifi_start) {
            wifi_start = false;
            ESP_ERROR_CHECK(esp_wifi_stop());
        }
    }

    void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
            ESP_LOGI(TAG_AP, "Station " MACSTR " joined, AID=%d",
                    MAC2STR(event->mac), event->aid);
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
            ESP_LOGI(TAG_AP, "Station " MACSTR " left, AID=%d, reason:%d",
                    MAC2STR(event->mac), event->aid, event->reason);

        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            wifi_config_t wifi_conf;
            esp_wifi_get_config(WIFI_IF_STA, &wifi_conf);
            if (strlen((char *)wifi_conf.sta.ssid) != 0) {
                ESP_LOGW(TAG_STA, "Saved SSID: %s", wifi_conf.sta.ssid);
                ESP_LOGW(TAG_STA, "Saved PASS: %s", wifi_conf.sta.password);
                esp_wifi_connect();
            }
            ESP_LOGI(TAG_STA, "Station started");
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if (!smart_conf_toggle) esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            _net_status_publisher(NetStatus::NO_IP);

        } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
            ESP_LOGI(TAG_SMART, "Scan done");
            xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_START_BIT);
        } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
            ESP_LOGI(TAG_SMART, "Found channel");
        } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
            ESP_LOGI(TAG_SMART, "Got SSID and password");

            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
            wifi_config_t wifi_config;
            uint8_t ssid[33] = { 0 };
            uint8_t password[65] = { 0 };
            uint8_t rvd_data[33] = { 0 };

            bzero(&wifi_config, sizeof(wifi_config_t));
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            memcpy(ssid, evt->ssid, sizeof(evt->ssid));
            memcpy(password, evt->password, sizeof(evt->password));
            ESP_LOGI(TAG_SMART, "SSID:%s", ssid);
            ESP_LOGI(TAG_SMART, "PASSWORD:%s", password);
            if (evt->type == SC_TYPE_ESPTOUCH_V2) {
                ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
                ESP_LOGI(TAG_SMART, "RVD_DATA:");
                for (int i=0; i<33; i++) {
                    printf("%02x ", rvd_data[i]);
                }
                printf("\n");
            }

            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
            esp_wifi_connect();
        } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
            xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);

        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
            ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            _net_status_publisher(NetStatus::GOT_IP);
        }
    }

    esp_netif_t *wifi_init_softap(void) {
        esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

        wifi_config_t wifi_ap_config = {
            .ap = {
                .ssid = ESP_WIFI_AP_SSID,
                .password = ESP_WIFI_AP_PASSWD,
                .ssid_len = strlen(ESP_WIFI_AP_SSID),
                .channel = ESP_WIFI_CHANNEL,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .max_connection = MAX_STA_CONN,
                .pmf_cfg = {
                    .required = false,
                },
            },
        };

        if (strlen(ESP_WIFI_AP_PASSWD) == 0) {
            wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

        ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
                ESP_WIFI_AP_SSID, ESP_WIFI_AP_PASSWD, ESP_WIFI_CHANNEL);

        return esp_netif_ap;
    }

    esp_netif_t *wifi_init_sta(WifiMode wifi_mode) {
        esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

        if (wifi_mode != WifiMode::SMART_CONFIG_MODE) {
            wifi_config_t wifi_sta_config = {
                .sta = {
                    .ssid = ESP_WIFI_STA_SSID,
                    .password = ESP_WIFI_STA_PASSWD,
                    .scan_method = WIFI_ALL_CHANNEL_SCAN,
                    .threshold = {
                        .authmode = WIFI_AUTH_WPA2_PSK,
                    },
                    .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
                    .failure_retry_cnt = ESP_MAXIMUM_RETRY,
                },
            };
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
        }

        ESP_LOGI(TAG_STA, "wifi_init_sta finished.");

        return esp_netif_sta;
    }

    void Wifi::init() {
        s_wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        &wifi_event_handler,
                        NULL,
                        NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                        IP_EVENT_STA_GOT_IP,
                        &wifi_event_handler,
                        NULL,
                        NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(SC_EVENT,
                        ESP_EVENT_ANY_ID,
                        &wifi_event_handler,
                        NULL,
                        NULL));

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    }

    void Wifi::start() {
        init();

        xTaskCreatePinnedToCore([](void *arg) { static_cast<Wifi *>(arg)->start_task(arg); },
            "Start wifi task", 1024 * 10, this, 5, NULL, 0
        );
    }

    void Wifi::start_task(void *pvParameters) {
        select_mode(WifiMode::SMART_CONFIG_MODE);

        ev_wifi.subscribe(EventID::BUTTON_SC, [this](void *data) { this->event_smartconfig_toggle(data); });
        vTaskDelete(NULL);
    }

    void softap_set_dns_addr(esp_netif_t *esp_netif_ap,esp_netif_t *esp_netif_sta) {
        esp_netif_dns_info_t dns;
        esp_netif_get_dns_info(esp_netif_sta,ESP_NETIF_DNS_MAIN,&dns);
        uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
        ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
        ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));
    }

    void Wifi::select_mode(WifiMode wifi_mode) {
        if (wifi_start) {
            wifi_start = false;
            ESP_ERROR_CHECK(esp_wifi_stop());
        }

        if (esp_netif_ap_) esp_netif_destroy(esp_netif_ap_);
        if (esp_netif_sta_) esp_netif_destroy(esp_netif_sta_);

        if (wifi_mode == WifiMode::AP_MODE) {
            ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
            esp_netif_ap_ = wifi_init_softap();

        } else if (wifi_mode == WifiMode::STA_MODE) {
            ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            esp_netif_sta_ = wifi_init_sta(WifiMode::STA_MODE);

        } else if (wifi_mode == WifiMode::APSTA_MODE) {
            ESP_LOGI(TAG_APSTA, "ESP_WIFI_MODE_APSTA");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
            esp_netif_sta_ = wifi_init_sta(WifiMode::STA_MODE);
            esp_netif_ap_ = wifi_init_softap();

        } else if (wifi_mode == WifiMode::SMART_CONFIG_MODE) {
            ESP_LOGI(TAG_SMART, "ESP_WIFI_MODE_SMART_CONFIG");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            esp_netif_sta_ = wifi_init_sta(WifiMode::SMART_CONFIG_MODE);
        }

        wifi_start = true;
        ESP_ERROR_CHECK(esp_wifi_start());

        if (wifi_mode == WifiMode::APSTA_MODE) {
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                                    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                                    pdFALSE,
                                                    pdFALSE,
                                                    portMAX_DELAY);

            if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG_STA, "connected to ap SSID: %s password: %s",
                        ESP_WIFI_STA_SSID, ESP_WIFI_STA_PASSWD);
                softap_set_dns_addr(esp_netif_ap_,esp_netif_sta_);
            } else if (bits & WIFI_FAIL_BIT) {
                ESP_LOGI(TAG_STA, "Failed to connect to SSID:%s, password:%s",
                        ESP_WIFI_STA_SSID, ESP_WIFI_STA_PASSWD);
            } else {
                ESP_LOGE(TAG_STA, "UNEXPECTED EVENT");
                return;
            }

            esp_netif_set_default_netif(esp_netif_sta_);

            if (esp_netif_napt_enable(esp_netif_ap_) != ESP_OK) {
                ESP_LOGE(TAG_STA, "NAPT not enabled on the netif: %p", esp_netif_ap_);
            }
        }
    }

    static void smartconfig_task(void * parm) {
        EventBits_t uxBits;
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
        ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
        while (1) {
            uxBits = xEventGroupWaitBits(s_wifi_event_group, ESPTOUCH_START_BIT | WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
            if (uxBits & ESPTOUCH_START_BIT) {
                smart_conf_start = true;
                _button_publisher(LedLevel::LEVEL_2);

            } else if (uxBits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG_SMART, "WiFi Connected to ap");

            } else if(uxBits & ESPTOUCH_DONE_BIT) {
                esp_smartconfig_stop();
                smart_conf_start = false;
                _button_publisher(LedLevel::LEVEL_0);

                ESP_LOGI(TAG_SMART, "smartconfig over");
                vTaskDelete(NULL);
            }
        }
    }

    void Wifi::start_smartconfig() {
        _button_publisher(LedLevel::LEVEL_1);

        esp_wifi_disconnect();
        xTaskCreate(smartconfig_task, "Start smartconfig task", 1024 * 5, this, 3, NULL);
    }

    void Wifi::stop_smartconfig() {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
        esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, NULL, 0, portMAX_DELAY);
        ESP_LOGI(TAG_SMART, "Smartconfig stopped");
    }

    void Wifi::event_smartconfig_toggle(void *data) {
        if (!smart_conf_toggle && !smart_conf_start) {
            smart_conf_toggle = true;
            start_smartconfig();
        } else if (smart_conf_start) {
            smart_conf_toggle = false;
            stop_smartconfig();
        } else ESP_LOGW(TAG_SMART, "Smartconfig starting");
    }

    void _button_publisher(LedLevel level) {
        mess_led = level;
        ev_wifi.publish(EventID::LED_SC, &mess_led);
    }

    void _net_status_publisher(NetStatus net) {
        mess_net = net;
        ev_wifi.publish(EventID::NET_STATUS, &mess_net);
    }

} // namespace network
