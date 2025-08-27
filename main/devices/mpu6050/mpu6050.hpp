#pragma once

#include "peripherals/gpio.hpp"
#include "peripherals/i2c.hpp"
#include "common/config.h"

namespace devices {
    class MPU6050 {
    public:
        MPU6050(peripherals::I2CDriver *i2c_driver, i2c_port_num_t i2c_port_num, uint16_t device_address, uint32_t i2c_freq_hz,
                EnableLog show_values_log);
        // ~MPU6050();

        void init();
        void start();

        uint8_t querry(uint8_t reg);

    private:
        peripherals::I2CDriver *i2c_driver_;
        i2c_master_dev_handle_t dev_handle_;
        i2c_port_num_t i2c_port_num_;
        uint16_t device_address_;
        uint32_t i2c_freq_hz_;
        EnableLog show_values_log_;

        uint8_t trans_buf_[2];
        uint8_t recv_buf_;

    }; // class MPU6050

} // namespace devices
