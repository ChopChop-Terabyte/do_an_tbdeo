#pragma once

#include "driver/i2c_master.h"

namespace peripherals {
    class I2C {
    public:
        I2C(i2c_port_num_t i2c_port_num, gpio_num_t sda_pin, gpio_num_t scl_pin);
        ~I2C();

        void init();
        void start();

        void scan_dev_address(i2c_port_num_t i2c_port_num);
        void add_dev(i2c_port_num_t i2c_port_num, i2c_master_dev_handle_t *dev_handle, uint16_t device_address, uint32_t i2c_freq_hz);
        void remove_dev(i2c_master_dev_handle_t dev_handle);
        void write_bytes(i2c_master_dev_handle_t dev_handle,
                                    uint8_t *write_buf, size_t read_size);
        void read_bytes(i2c_master_dev_handle_t dev_handle,
                                    uint8_t *read_buf, size_t read_size);
        void write_read(i2c_master_dev_handle_t dev_handle,
                        uint8_t *write_buf, size_t write_size,
                        uint8_t *read_buf, size_t read_size);

    private:
        i2c_port_num_t i2c_port_num_;
        gpio_num_t sda_pin_;
        gpio_num_t scl_pin_;

    }; // class I2C

} // namespace peripherals
