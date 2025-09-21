#pragma once

#include "driver/spi_master.h"
#include "peripherals/gpio.hpp"

namespace peripherals {
    class SPI {
    public:
        SPI(spi_host_device_t spi_host, gpio_num_t mosi_pin, gpio_num_t miso_pin, gpio_num_t clk_pin);
        ~SPI() {}

        void init();
        void start();

        void add_dev(spi_host_device_t spi_host, spi_device_handle_t *dev_handle, gpio_num_t cs_pin, int spi_clock_hz);
        // void remove_dev(i2c_master_dev_handle_t dev_handle);
        void write_bytes(spi_device_handle_t dev_handle, const uint8_t *write_buf, size_t write_size);
        // void read_bytes(i2c_master_dev_handle_t dev_handle,
        //                             uint8_t *read_buf, size_t read_size);
        // void write_read(i2c_master_dev_handle_t dev_handle,
        //                 uint8_t *write_buf, size_t write_size,
        //                 uint8_t *read_buf, size_t read_size);

    private:
        spi_host_device_t spi_host_;
        gpio_num_t mosi_pin_;
        gpio_num_t miso_pin_;
        gpio_num_t clk_pin_;

    }; // class SPI

} // namespace peripherals
