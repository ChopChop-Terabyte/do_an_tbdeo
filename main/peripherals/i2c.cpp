#include "i2c.h"
#include "esp_log.h"

static const char *TAG = "I2C";

namespace peripherals {
    static bool i2c_bus_initialized[2] = {false, false};
    static i2c_master_bus_handle_t bus_handle[2] = {nullptr, nullptr};

    I2C::I2C(i2c_port_num_t i2c_port_num, gpio_num_t sda_pin, gpio_num_t scl_pin)
            : i2c_port_num_(i2c_port_num),
            sda_pin_(sda_pin),
            scl_pin_(scl_pin) {}

    I2C::~I2C() {
        for (uint8_t i = 0; i < 2; i++) {
            if (i2c_bus_initialized[i]) {
                ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle[i]));
                i2c_bus_initialized[i] = false;

                ESP_LOGI(TAG, "I2C port num %d cleared.", i);
            }
        }
    }

    void I2C::init() {
        if (!i2c_bus_initialized[i2c_port_num_]) {
            i2c_master_bus_config_t bus_config = {
                .i2c_port = i2c_port_num_,
                .sda_io_num = sda_pin_,
                .scl_io_num = scl_pin_,
                .clk_source = I2C_CLK_SRC_DEFAULT,
                .glitch_ignore_cnt = 7,
                .flags = {
                    .enable_internal_pullup = true,
                },
            };
            ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle[i2c_port_num_]));

            i2c_bus_initialized[i2c_port_num_] = true;
            ESP_LOGI(TAG, "I2C port num %d initialized.", i2c_port_num_);
        } else ESP_LOGW(TAG, "I2C port number %d has been previously initialized.", i2c_port_num_);
    }

    void I2C::start() {
        init();
    }

    void I2C::scan_dev_address(i2c_port_num_t i2c_port_num) {
        bool first = true;
        ESP_LOGI(TAG, "Scanning I2C bus...");

        for (uint8_t addr = 1; addr < 0x7F; addr++) {
            esp_err_t ret = i2c_master_probe(bus_handle[i2c_port_num], addr, 1000); // timeout = 1000us
            if (ret == ESP_OK) {
                if (first) {
                    first = false;
                    ESP_LOGI(TAG, "***** I2C address list *****");
                }
                ESP_LOGI(TAG, "Found device at 0x%02X", addr);
            }
        }

        ESP_LOGI(TAG, "*****     End list     *****");
        ESP_LOGI(TAG, "Scan done");
    }

    void I2C::add_dev(i2c_port_num_t i2c_port_num, i2c_master_dev_handle_t *dev_handle, uint16_t device_address, uint32_t i2c_freq_hz) {
        if (i2c_bus_initialized[i2c_port_num]) {
            i2c_device_config_t dev_config = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = device_address,
                .scl_speed_hz = i2c_freq_hz,
            };
            ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle[i2c_port_num], &dev_config, dev_handle));
        } else ESP_LOGW(TAG, "I2C port num %d is not initialized", i2c_port_num);
    }

    void I2C::remove_dev(i2c_master_dev_handle_t dev_handle) {
        ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    }

    void I2C::write_bytes(i2c_master_dev_handle_t dev_handle, uint8_t *write_buf, size_t write_size) {
        i2c_master_transmit(dev_handle, write_buf, write_size, -1);
    }

    void I2C::read_bytes(i2c_master_dev_handle_t dev_handle, uint8_t *read_buf, size_t read_size) {
        i2c_master_receive(dev_handle, read_buf, read_size, -1);
    }

    void I2C::write_read(i2c_master_dev_handle_t dev_handle, uint8_t *write_buf, size_t write_size, uint8_t *read_buf, size_t read_size) {
        i2c_master_transmit_receive(dev_handle, write_buf, write_size, read_buf, read_size, -1);
    }

} // namespace peripherals
