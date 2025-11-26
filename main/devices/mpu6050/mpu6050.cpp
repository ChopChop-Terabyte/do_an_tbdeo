#include "mpu6050.h"
#include "esp_log.h"

static const char *TAG = "MPU6050";

namespace devices {
    bool MPU6050::new_val = false;

    MPU6050::MPU6050(peripherals::I2C *i2c_driver, i2c_port_num_t i2c_port_num, uint16_t device_address, uint32_t i2c_freq_hz,
                    EnableLog show_values_log)
                        : i2c_driver_(i2c_driver),
                        i2c_port_num_(i2c_port_num),
                        device_address_(device_address),
                        i2c_freq_hz_(i2c_freq_hz),
                        show_values_log_(show_values_log) {}

    void MPU6050::init() {
        i2c_driver_->add_dev(i2c_port_num_, &dev_handle_, device_address_, i2c_freq_hz_);
        ESP_LOGI(TAG, "MPU6050 Added");

        while (true) {
            if (querry(WHO_AM_I) == 0x70) {
                config();
                ESP_LOGI(TAG, "MPU6050 ready.");
                break;
            } else {
                ESP_LOGW(TAG, "Waitting MPU6050.");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
    }

    void MPU6050::start() {
        init();

        xTaskCreatePinnedToCore([](void *arg) { static_cast<MPU6050 *>(arg)->start_task(arg); },
            "Start MAX30102 task", 1024 * 4, this, 2, &sensor_task, 1
        );
    }

    void MPU6050::start_task(void *pvParameters) {
        while (true) {
            if (querry(INT_STATUS) & 1) {
                querry();
            }

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }

    uint8_t MPU6050::querry(uint8_t reg) {
        uint8_t data;
        trans_buf_[0] = reg;
        i2c_driver_->write_read(dev_handle_, trans_buf_, 1, &data, 1);

        return data;
    }

    void MPU6050::querry() {
        uint8_t data[6];
        trans_buf_[0] = ACCEL_X_H;
        trans_buf_[1] = GYRO_X_H;

        i2c_driver_->write_read(dev_handle_, trans_buf_, 1, data, 6);
        accel_x_ = (data[0] << 8) | data[1];
        accel_y_ = (data[2] << 8) | data[3];
        accel_z_ = (data[4] << 8) | data[5];

        i2c_driver_->write_read(dev_handle_, &trans_buf_[1], 1, data, 6);
        gyro_x_ = (data[0] << 8) | data[1];
        gyro_y_ = (data[2] << 8) | data[3];
        gyro_z_ = (data[4] << 8) | data[5];

        // ESP_LOGW(TAG, "%d", accel_x_);
        // ESP_LOGW(TAG, "%d", accel_y_);
        // ESP_LOGW(TAG, "%d", accel_z_);
        // ESP_LOGE(TAG, "-----------------------------");
        // ESP_LOGW(TAG, "%d", gyro_x_);
        // ESP_LOGW(TAG, "%d", gyro_y_);
        // ESP_LOGW(TAG, "%d", gyro_z_);
        // ESP_LOGE(TAG, "-----------------------------");

        new_val = true;
    }

    void MPU6050::config(uint8_t reg, uint8_t option) {
        trans_buf_[0] = reg;
        trans_buf_[1] = option;
        i2c_driver_->write_bytes(dev_handle_, trans_buf_, 2);
    }

    void MPU6050::config() {
        config(PWR_MGMT_1, RESET);
        config(PWR_MGMT_1, 0x08);
        config(PWR_MGMT_1, 0x0B);
        config(CONFIG, 0x06);
        config(SMPRT_DIV, 0xC7);
        config(GYRO_CONFIG, 0x18);
        config(ACCEL_CONFIG, 0x18);

        // ESP_LOGW(TAG, "0x%02X", querry(GYRO_CONFIG));
        // ESP_LOGW(TAG, "0x%02X", querry(ACCEL_CONFIG));
        // ESP_LOGW(TAG, "0x%02X", querry(CONFIG));
    }

} // namespace devices
