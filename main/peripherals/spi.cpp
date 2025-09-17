#include "spi.hpp"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "SPI";

namespace peripherals {
    static bool spi_host_initialized[2] = {false, false};

    SPI::SPI(spi_host_device_t spi_host, gpio_num_t mosi_pin, gpio_num_t miso_pin, gpio_num_t clk_pin)
            : spi_host_(spi_host),
            mosi_pin_(mosi_pin),
            miso_pin_(miso_pin),
            clk_pin_(clk_pin) {}

    void SPI::init() {
        if (!spi_host_initialized[spi_host_-1]) {
            spi_bus_config_t buscfg = {
                .mosi_io_num = mosi_pin_,
                .miso_io_num = miso_pin_,
                .sclk_io_num = clk_pin_,
                .quadwp_io_num = -1,
                .quadhd_io_num = -1,
                .max_transfer_sz = 1024
            };
            ESP_ERROR_CHECK(spi_bus_initialize(spi_host_, &buscfg, SPI_DMA_CH1));

            spi_host_initialized[spi_host_-1] = true;
            ESP_LOGI(TAG, "SPI host %d initialized.", spi_host_);
        }
    }

    void SPI::start() {
        init();
    }

    void SPI::add_dev(spi_host_device_t spi_host, spi_device_handle_t *dev_handle, gpio_num_t cs_pin, int spi_clock_hz) {
        if (spi_host_initialized[spi_host-1]) {
            spi_device_interface_config_t devcfg = {
                .mode = 0,
                .clock_speed_hz = spi_clock_hz,
                .spics_io_num = cs_pin,
                .queue_size = 7,
                .pre_cb = nullptr,
            };
            ESP_ERROR_CHECK(spi_bus_add_device(spi_host, &devcfg, dev_handle));
        } else ESP_LOGW(TAG, "SPI host %d is not initialized", spi_host);
    }

    void SPI::write_bytes(spi_device_handle_t dev_handle, uint8_t *write_buf, size_t write_size) {
        spi_transaction_t trans;
        memset(&trans, 0, sizeof(trans));

        trans.length = write_size * 8;
        trans.tx_buffer = write_buf;

        spi_device_polling_transmit(dev_handle, &trans);
    }

}
