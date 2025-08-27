#include "mpu6050.hpp"
#include "esp_log.h"

static const char *TAG = "MPU6050";

namespace devices {
    MPU6050::MPU6050(peripherals::I2CDriver *i2c_driver, i2c_port_num_t i2c_port_num, uint16_t device_address, uint32_t i2c_freq_hz,
                    EnableLog show_values_log)
                        : i2c_driver_(i2c_driver),
                        i2c_port_num_(i2c_port_num),
                        device_address_(device_address),
                        i2c_freq_hz_(i2c_freq_hz),
                        show_values_log_(show_values_log) {}

    void MPU6050::init() {
        i2c_driver_->add_dev(i2c_port_num_, &dev_handle_, device_address_, i2c_freq_hz_);
        ESP_LOGI(TAG, "MPU6050 Added");
    }

    void MPU6050::start() {
        init();

        ESP_LOGW(TAG, "0x%02X", querry(0x75));
    }

    uint8_t MPU6050::querry(uint8_t reg) {
        uint8_t data;
        trans_buf_[0] = reg;
        i2c_driver_->write_read(dev_handle_, trans_buf_, 1, &data, 1);

        return data;
    }

} // namespace devices
