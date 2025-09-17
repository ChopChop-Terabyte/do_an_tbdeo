#pragma once

#include "peripherals/spi.hpp"
#include "core/sntp.h"

namespace devices {
    class SH1106 {
    public:
        TaskHandle_t time_clock_task_ = nullptr;

        SH1106(peripherals::SPI *spi_driver, spi_host_device_t spi_host, gpio_num_t cs_pin,
                gpio_num_t dc_pin, gpio_num_t res_pin, int spi_clock_hz);

        void init();
        void start();

        void time_clock_task(void *pvParameters);
        void set_cursor(uint8_t page, uint8_t col);
        void cmd_tran(uint8_t write_buf);
        void data_tran(uint8_t *write_buf, size_t write_size);
        void config();
        void clean();
        void render_text(uint8_t font, uint8_t page, uint8_t col, uint8_t *data);
        void render(uint8_t *pic);

    private:
        static constexpr uint8_t DISP_OFF = 0xAE;
        static constexpr uint8_t DISP_ON = 0xAF;
        static constexpr uint8_t NORMAL_DISP = 0xA4;
        static constexpr uint8_t FULL_DISP = 0xA5;

        peripherals::SPI *spi_driver_;
        spi_host_device_t spi_host_;
        gpio_num_t cs_pin_;
        gpio_num_t dc_pin_;
        gpio_num_t res_pin_;
        int spi_clock_hz_;
        spi_device_handle_t dev_handle_;

    }; // class SH1106

} // namespace devices
