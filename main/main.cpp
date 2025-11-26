#include <memory>
#include <string>
// #include <sstream>
// #include <iomanip>
#include <nlohmann/json.hpp>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "core/info.h"
#include "common/config.h"
#include "peripherals/gpio.h"
#include "peripherals/i2c.h"
#include "peripherals/spi.h"
#include "network/net_manager.h"
#include "core/ota.h"
#include "protocols/mqtt/mqtt.h"
#include "core/event_manager.h"

#include "devices/max30102/max30102.h"
#include "devices/mpu6050/mpu6050.h"
#include "devices/oled/sh1106.h"

static const char *TAG = "Main task";

using json = nlohmann::json;
using namespace core;
using namespace peripherals;
using namespace network;
using namespace protocols;
using namespace devices;

auto gpio_ = std::make_unique<GPIO>();
auto i2c_ = std::make_unique<I2C>(I2C_BUS_0, SDA_PIN, SCL_PIN);
auto spi_ = std::make_unique<SPI>(SPI_HOST_0, SPI_MOSI, SPI_MISO, SPI_CLK);
auto net_manager_ = std::make_unique<NetManager>();
auto ota_ = std::make_unique<OTA>();
auto mqtt_ = std::make_unique<MQTT>(SERVER_ADDRESS, PORT, MQTT_TRANSPORT_OVER_TCP);
auto &event_manager_ = EventManager::instance();

auto max30102_ = std::make_unique<MAX30102>(i2c_.get(), I2C_BUS_0, MAX30102_ADDRESS, MAX30102_FREQ_HZ, EnableLog::SHOW_ON);
auto mpu6050_ = std::make_unique<MPU6050>(i2c_.get(), I2C_BUS_0, MPU6050_ADDRESS, MPU6050_FREQ_HZ, EnableLog::SHOW_ON);
auto sh1106_ = std::make_unique<SH1106>(spi_.get(), SPI_HOST_0, SH1106_CS, SH1106_DC, SH1106_RES, SH1106_FREQ_HZ);

extern "C" void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_->start();
    i2c_->start();
    spi_->start();
    net_manager_->start();
    ota_->start();
    mqtt_->start();
    event_manager_.start();

    max30102_->start();
    // mpu6050_->start();
    // vTaskDelay(500 / portTICK_PERIOD_MS);
    sh1106_->start();

    // i2c_->scan_dev_address(I2C_BUS_0);

    uint8_t i = 0;
    while (true) {
        json json_puber_1;
        // json json_puber_2;
        json_puber_1[ID] = p_info.id;

        if (!i) {
            ESP_LOGI(TAG, "[APP] Free memory:           %" PRIu32 " bytes", esp_get_free_heap_size());
            ESP_LOGI(TAG, "[APP] Internal free heap:    %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        }

        if (mqtt_->is_connected_ && max30102_->is_new_val()) {
            int bpm_val = max30102_->get_heart_rate();
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << max30102_->get_spo2();

            json_puber_1[BPM] = std::to_string(bpm_val);
            json_puber_1[SPO2] = oss.str();

            std::string mess = json_puber_1.dump();

            mqtt_->publish(TOPIC_CENTER_SENSOR, mess.c_str());
            // ESP_LOGW(TAG, "%s", mess.c_str());
        }

        // if (mqtt_->is_connected_ && mpu6050_->is_new_val()) {
        //     json_puber_2[ACCEL_X] = std::to_string(mpu6050_->get_accel_x());
        //     json_puber_2[ACCEL_Y] = std::to_string(mpu6050_->get_accel_y());
        //     json_puber_2[ACCEL_Z] = std::to_string(mpu6050_->get_accel_z());
        //     json_puber_2[GYRO_X] = std::to_string(mpu6050_->get_gyro_x());
        //     json_puber_2[GYRO_Y] = std::to_string(mpu6050_->get_gyro_y());
        //     json_puber_2[GYRO_Z] = std::to_string(mpu6050_->get_gyro_z());

        //     std::string mess = json_puber_2.dump();

        //     mqtt_->publish(mqtt_->client_, TOPIC_CENTER_CONNECT, mess.c_str());
        //     // ESP_LOGW(TAG, "%s", mess.c_str());
        // }

        if (i++ > 50) i = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
