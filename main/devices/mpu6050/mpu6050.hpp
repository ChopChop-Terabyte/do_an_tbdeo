#pragma once

#include "peripherals/gpio.hpp"
#include "peripherals/i2c.hpp"
#include "common/config.h"

namespace devices {
    class MPU6050 {
    public:
        TaskHandle_t sensor_task = nullptr;

        MPU6050(peripherals::I2C *i2c_driver, i2c_port_num_t i2c_port_num, uint16_t device_address, uint32_t i2c_freq_hz,
                EnableLog show_values_log);
        // ~MPU6050();

        void init();
        void start();

        void start_task(void *pvParameters);
        uint8_t querry(uint8_t reg);
        void querry();
        void config(uint8_t reg, uint8_t option);
        void config();
        int16_t get_accel_x() { return accel_x_; }
        int16_t get_accel_y() { return accel_y_; }
        int16_t get_accel_z() { return accel_z_; }
        int16_t get_gyro_x() { return gyro_x_; }
        int16_t get_gyro_y() { return gyro_y_; }
        int16_t get_gyro_z() { return gyro_z_; }

        bool is_new_val() {
            if (new_val) {
                new_val = false;
                return true;
            }
            return false;
        }

    private:
        static constexpr uint8_t WHO_AM_I = 0x75;

    // Sample Rate Divider
        static constexpr uint8_t SMPRT_DIV = 0x19;          // Sample Rate Divider register

    // Power Management 1
        static constexpr uint8_t PWR_MGMT_1 = 0x6B;         // Power Management 1 register
        static constexpr uint8_t RESET = 0x80;

    // Configuration
        static constexpr uint8_t CONFIG = 0x1A;             // Configuration register
        
    // Gyroscope Configuration
        static constexpr uint8_t GYRO_CONFIG = 0x1B;        // Gyroscope Configuration register

    // Interrupt Status
        static constexpr uint8_t INT_STATUS = 0x3A;         // Interrupt Status register

    // Accelerometer Configuration
        static constexpr uint8_t ACCEL_CONFIG = 0x1C;       // Accelerometer Configuration register

    // Accelerometer Measurements
        static constexpr uint8_t ACCEL_X_H = 0x3B;          // Accelerometer Measurements X H-bits register
        static constexpr uint8_t ACCEL_X_L = 0x3C;          // Accelerometer Measurements X L-bits register
        static constexpr uint8_t ACCEL_Y_H = 0x3D;          // Accelerometer Measurements Y H-bits register
        static constexpr uint8_t ACCEL_Y_L = 0x3E;          // Accelerometer Measurements Y L-bits register
        static constexpr uint8_t ACCEL_Z_H = 0x3F;          // Accelerometer Measurements Z H-bits register
        static constexpr uint8_t ACCEL_Z_L = 0x40;          // Accelerometer Measurements Z L-bits register

    // Gyroscope Measurements
        static constexpr uint8_t GYRO_X_H = 0x43;           // Gyroscope Measurements X H-bits register
        static constexpr uint8_t GYRO_X_L = 0x44;           // Gyroscope Measurements X L-bits register
        static constexpr uint8_t GYRO_Y_H = 0x45;           // Gyroscope Measurements Y H-bits register
        static constexpr uint8_t GYRO_Y_L = 0x46;           // Gyroscope Measurements Y L-bits register
        static constexpr uint8_t GYRO_Z_H = 0x47;           // Gyroscope Measurements Z H-bits register
        static constexpr uint8_t GYRO_Z_L = 0x48;           // Gyroscope Measurements Z L-bits register

        peripherals::I2C *i2c_driver_;
        i2c_master_dev_handle_t dev_handle_;
        i2c_port_num_t i2c_port_num_;
        uint16_t device_address_;
        uint32_t i2c_freq_hz_;
        EnableLog show_values_log_;

        uint8_t trans_buf_[2];
        uint8_t recv_buf_;

        int16_t accel_x_;
        int16_t accel_y_;
        int16_t accel_z_;
        int16_t gyro_x_;
        int16_t gyro_y_;
        int16_t gyro_z_;
        static bool new_val;

    }; // class MPU6050

} // namespace devices
