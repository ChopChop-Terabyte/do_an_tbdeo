#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "common/config.h"
#include "peripherals/gpio.hpp"
#include "peripherals/i2c.hpp"
#include "network/net_manager.hpp"
#include "protocols/mqtt/mqtt.hpp"
#include "core/event_manager.hpp"

#include "devices/max30102/max30102.hpp"
#include "devices/mpu6050/mpu6050.hpp"

static const char *TAG = "Main task";

using json = nlohmann::json;
using namespace core;
using namespace peripherals;
using namespace network;
using namespace protocols;
using namespace devices;

auto gpio_ = std::make_unique<GPIO>();
auto i2c_ = std::make_unique<I2CDriver>(I2C_BUS_0, SDA_PIN, SCL_PIN);
auto net_manager_ = std::make_unique<NetManager>();
auto mqtt_ = std::make_unique<MQTT>(SERVER_ADDRESS, PORT, MQTT_TRANSPORT_OVER_TCP);
auto &event_manager_ = EventManager::instance();

auto max30102_ = std::make_unique<MAX30102>(i2c_.get(), I2C_BUS_0, MAX30102_ADDRESS, MAX30102_FREQ_HZ, EnableLog::SHOW_OFF);
auto mpu6050_ = std::make_unique<MPU6050>(i2c_.get(), I2C_BUS_0, MPU6050_ADDRESS, MPU6050_FREQ_HZ, EnableLog::SHOW_ON);

extern "C" void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_->start();
    i2c_->start();
    net_manager_->start();
    mqtt_->start();
    event_manager_.start();

    max30102_->start();
    mpu6050_->start();

    while (true) {
        json json_puber;

        if (mqtt_->is_connected_ && max30102_->is_new_val()) {
            json_puber[SPO2] = max30102_->get_spo2();
            json_puber[BPM] = max30102_->get_heart_rate();

            std::string mess = json_puber.dump();

            mqtt_->publish(mqtt_->client_, TOPIC_PUB_1, mess.c_str());
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // while (true) {
    //     ESP_LOGI(TAG, "[APP] Free memory:           %" PRIu32 " bytes", esp_get_free_heap_size());
    //     ESP_LOGI(TAG, "[APP] Internal free heap:    %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    //     vTaskDelay(5000 / portTICK_PERIOD_MS);
    // }
}
