#include "sh1106.hpp"
#include "esp_log.h"
#include "string.h"

#include "font_oled.h"

static const char *TAG = "SH1106";

using namespace peripherals;

namespace devices {
    SH1106::SH1106(peripherals::SPI *spi_driver, spi_host_device_t spi_host, gpio_num_t cs_pin,
                    gpio_num_t dc_pin, gpio_num_t res_pin, int spi_clock_hz)
                        : spi_driver_(spi_driver),
                        spi_host_(spi_host),
                        cs_pin_(cs_pin),
                        dc_pin_(dc_pin),
                        res_pin_(res_pin),
                        spi_clock_hz_(spi_clock_hz) {}

    void SH1106::init() {
        GPIO::output_config(dc_pin_, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE);
        GPIO::output_config(res_pin_, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE);
        gpio_set_level(res_pin_, 0);

        spi_driver_->add_dev(spi_host_, &dev_handle_, cs_pin_, spi_clock_hz_);

        config();
        ESP_LOGI(TAG, "OLED ready.");
    }

    void SH1106::start() {
        init();
        sntp_config();

        xTaskCreatePinnedToCore([](void *arg) { static_cast<SH1106 *>(arg)->time_clock_task(arg); },
            "Start time clock task", 1024 * 8, this, 10, &time_clock_task_, 1
        );
    }

    void SH1106::time_clock_task(void *pvParameters) {
        uint8_t open_eyes[15] = {0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00, 0x40, 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00};
        uint8_t close_eyes[15] = {0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00, 0x40, 0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00};
        uint8_t tick = 0;

        set_cursor(0, 113);
        data_tran(open_eyes, sizeof(open_eyes));

        while (true) {
            char buffer[16];

            get_time();
            snprintf(buffer, 11, "%04d-%02d-%02d", year, mon, mday);
            render_text(0, 0, 0, (uint8_t *)buffer);
            snprintf(buffer, 6, "%02d:%02d", hour, min);
            render_text(0, 0, 71, (uint8_t *)buffer);
            ESP_LOGE(TAG,"%s", buffer);

            if (tick++ > 5) {
                tick = 0;
                set_cursor(0, 113);
                data_tran(close_eyes, sizeof(close_eyes));
                vTaskDelay(100 / portTICK_PERIOD_MS);
                set_cursor(0, 113);
                data_tran(open_eyes, sizeof(open_eyes));
            }
            vTaskDelay(850 / portTICK_PERIOD_MS);
        }
        vTaskDelete(NULL);
    }

    void SH1106::set_cursor(uint8_t page, uint8_t col) {
        cmd_tran(0xB0 + page);
        cmd_tran(0x00 + ((col + 2) & 0x0F));
        cmd_tran(0x10 + (((col + 2) >> 4) & 0x0F));
    }

    void SH1106::cmd_tran(uint8_t write_buf) {
        gpio_set_level(dc_pin_, 0);
        spi_driver_->write_bytes(dev_handle_, &write_buf, 1);
    }

    void SH1106::data_tran(uint8_t *write_buf, size_t write_size) {
        gpio_set_level(dc_pin_, 1);
        spi_driver_->write_bytes(dev_handle_, write_buf, write_size);
    }

    void SH1106::config() {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        gpio_set_level(res_pin_, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        cmd_tran(DISP_OFF);

        /* Oscillator Frequency */
        cmd_tran(0xD5); // - Set Display Clock Divide Ratio/Oscillator Frequency:
        cmd_tran(0x80); //     FOSC (Oscillator Frequency) = 8, Divide ratio = 1
        // cmd_tran(0xF0); //     FOSC (Oscillator Frequency) = 16, Divide ratio = 1
        // cmd_tran(0x0F); //     FOSC (Oscillator Frequency) = 0, Divide ratio = 16

        /* Default */
        cmd_tran(0x40); // Set Display Start Line
        // cmd_tran(0xA0); // Set Segment Re-map (column address 0 is mapped to SEG0 (RESET) )
        cmd_tran(0xA1); // Set Segment Re-map (column address 127 is mapped to SEG0)
        // cmd_tran(0xC0); // Set COM Output Scan Direction (0xC0 COM[0] - COM[N-1])
        cmd_tran(0xC8); // Set COM Output Scan Direction (0xC0 COM[N-1] - COM[0])
        cmd_tran(0xA8); // - Set Multiplex Ratio:
        cmd_tran(0x3F); //     64 MUX
        cmd_tran(0xD3); // - Set Display Offset:
        cmd_tran(0x00); //     0
        cmd_tran(0xDA); // - Set COM Pins Hardware Configuration:
        cmd_tran(0x12); //     (Alternative COM pin configuration) (Disable COM Left/Right remap)
        cmd_tran(0xDB); // - Set VCOMH Deselect Level:
        cmd_tran(0x40); //     Default
        // cmd_tran(0x00); //     ~0.65 x Vcc
        // cmd_tran(0x20); //     ~0.77 x Vcc
        // cmd_tran(0x30); //     ~0.83 x Vcc
        cmd_tran(0x8D); // - Charge Pump Setting:
        cmd_tran(0x14); //     ON
        cmd_tran(0x2E); // Deactivate Scroll

        /* Custom */
        cmd_tran(0x20); // - Set Memory Addressing Mode:
        cmd_tran(0x02); //     Page Addressing Mode (RESET)
        cmd_tran(0x00); // Set Lower Column Start Address for Page Addressing Mode
        cmd_tran(0x10); // Set Higher Column Start Address for Page Addressing Mode
        cmd_tran(0xA6); // Set Normal Display
        // cmd_tran(0xA7); // Set Inverse Display
        cmd_tran(0x81); // - Set Screen Contrast:
        cmd_tran(0xFF); //     255

        cmd_tran(DISP_ON);

        cmd_tran(NORMAL_DISP);
        // cmd_tran(FULL_DISP);
        clean();

        vTaskDelay(120 / portTICK_PERIOD_MS);
    }

    void SH1106::clean() {
        uint8_t cl[1056] = {0};

        for (uint8_t i = 0; i < 8; i++) {
            cmd_tran(0xB0 + i);
            cmd_tran(0x00);
            cmd_tran(0x10);

            data_tran(&cl[i*132], 132);
        }
    }

    void SH1106::render_text(uint8_t font, uint8_t page, uint8_t col, uint8_t *data) {
        cmd_tran(0xB0 + page);
        cmd_tran(0x00 + ((col + 2) & 0x0F));
        cmd_tran(0x10 + (((col + 2) >> 4) & 0x0F));

        for (uint8_t i = 0; i < strlen((char *)data); i++) {
            const uint8_t *c = get_font(font, data[i]);
            uint8_t buf[6];
            memcpy(buf, c, 6);

            data_tran(buf, 6);
        }
    }

    void SH1106::render(uint8_t *pic) {
        for (uint8_t i = 0; i < 8; i++) {
            cmd_tran(0xB0 + i);
            cmd_tran(0x02);
            cmd_tran(0x10);

            data_tran(&pic[i*128], 128);
        }
    }

} // namespace devices
