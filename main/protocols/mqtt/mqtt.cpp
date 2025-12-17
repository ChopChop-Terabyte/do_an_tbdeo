#include "mqtt.h"
#include "esp_log.h"

#include "core/event_manager.h"
#include <nlohmann/json.hpp>
#include "core/info.h"
#include "common/config.h"

static const char *TAG = "MQTT server";

using namespace core;
using json = nlohmann::json;

namespace protocols {
    auto &ev_mqtt = EventManager::instance();
    bool MQTT::is_connected_ = false;

    MQTT::MQTT(std::string server_address, uint32_t port, esp_mqtt_transport_t transport) : server_address_(server_address),
                                                                                port_(port),
                                                                                transport_(transport) {}

    MQTT::~MQTT() {}

    static void log_error_if_nonzero(const char *message, int error_code) {
        if (error_code != 0) {
            ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
        }
    }

    static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
        ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
        MQTT *mqtt = static_cast<MQTT *>(handler_args);
        esp_mqtt_event_handle_t event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);
        esp_mqtt_client_handle_t client = event->client;
        int msg_id;
        json data;
        std::string mess;

        switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt->is_connected_ = true;
            mqtt->subscribe_list();
            mqtt->info_pub();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt->is_connected_ = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            mqtt->on_data(event_data);
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
        }
    }

    void MQTT::init() {
        esp_mqtt_client_config_t mqtt_cfg = {
            .broker = {
                .address = {
                    .hostname = server_address_.c_str(),
                    .transport = transport_,
                    .port = port_,
                },
            },
            .network = {
                .reconnect_timeout_ms = 3000,
                .disable_auto_reconnect = false,
            },
        };

        client_ = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client_, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, this);
        esp_mqtt_client_start(client_);
    }

    void MQTT::start() {
        init();

        xTaskCreatePinnedToCore([](void *arg) { static_cast<MQTT *>(arg)->start_task(arg); },
            "Start mqtt task", 1024 * 2, this, 3, NULL, 1
        );
    }

    void MQTT::start_task(void *pvParameters) {
        json data;
        std::string mess;

        while (true) {
            if (is_connected_) {
                data[PING] = "1";
                mess = data.dump();
                publish(TOPIC_CENTER_CONNECT, mess.c_str());
            }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    void MQTT::subscribe_list() {
        esp_mqtt_client_subscribe(client_, TOPIC_CLIENT_CONNECT, 0);
        esp_mqtt_client_subscribe(client_, TOPIC_CLIENT_INFO, 0);
        esp_mqtt_client_subscribe(client_, TOPIC_CLIENT_NOTICE, 0);
        esp_mqtt_client_subscribe(client_, TOPIC_CLIENT_OTA, 0);
    }

    int MQTT::publish(const char *topic, const char *data) {
        return esp_mqtt_client_publish(client_, topic, data, strlen(data), 0, 0);
    }

    void MQTT::on_data(void *event_data) {
        esp_mqtt_event_handle_t event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);
        esp_mqtt_client_handle_t client = event->client;
        json data;
        std::string mess;

        if (strncmp(event->topic, TOPIC_CLIENT_CONNECT, event->topic_len) == 0) {
            info_pub();
        } else if (strncmp(event->topic, TOPIC_CLIENT_INFO, event->topic_len) == 0) {
            std::string data_str(event->data, event->data_len);
            data = json::parse(data_str);

            p_info.id = data[ID];
            p_info.name = data[NAME];
            p_info.gender = data[GENDER];
            p_info.age = data[AGE];
            p_info.weight = data[WEIGHT];
            p_info.height = data[HEIGHT];

            upadte = true;
            info_pub();
        } else if (strncmp(event->topic, TOPIC_CLIENT_NOTICE, event->topic_len) == 0) {
            std::string data_str(event->data, event->data_len);
            data = json::parse(data_str);
            int noti = std::stoi(data[NOTICE].get<std::string>());

            ev_mqtt.publish_intr(EventID::BUZZER, noti);
        } else if (strncmp(event->topic, TOPIC_CLIENT_OTA, event->topic_len) == 0) {
            std::string data_str(event->data, event->data_len);
            data = json::parse(data_str);
            static std::string url = data[URL];

            ev_mqtt.publish(EventID::OTA, &url);
        }
    }

    void MQTT::info_pub() {
        json data;
        std::string mess;

        data[ID] = p_info.id;
        data[NAME] = p_info.name;
        data[GENDER] = p_info.gender;
        data[AGE] = p_info.age;
        data[WEIGHT] = p_info.weight;
        data[HEIGHT] = p_info.height;
        mess = data.dump();

        publish(TOPIC_CENTER_INFO, mess.c_str());
    }

} // namespace protocols
