#include "sh1106.hpp"
#include "esp_log.h"
#include "string.h"

#include "core/event_manager.hpp"
#include "devices/max30102/max30102.hpp"
#include "font_oled.h"

static const char *TAG = "SH1106";

using namespace core;

namespace devices {
    auto &ev_sh1106 = EventManager::instance();
    LedLevel SH1106::smartconfig_level_ = LedLevel::LEVEL_0;
    static bool setup = false;
    static bool is_net_connected_ = false;

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

        ev_sh1106.subscribe(EventID::LED_SC, [this](void *data) { this->event_smartconfig_screen(data); });
        ev_sh1106.subscribe(EventID::NET_STATUS, [this](void *data) { this->event_network_status(data); });

        ESP_LOGI(TAG, "OLED ready.");
        setup = true;
    }

    void SH1106::start() {
        init();
        render(logo_iuh);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        clean();

        sntp_config();

        xTaskCreatePinnedToCore([](void *arg) { static_cast<SH1106 *>(arg)->time_clock_task(arg); },
            "Start time clock task", 1024 * 8, this, 10, &time_clock_task_, 0
        );
    }

    void SH1106::time_clock_task(void *pvParameters) {
        char buffer[100];
        uint8_t open_eyes[15] = {0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00, 0x40, 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00};
        uint8_t close_eyes[15] = {0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00, 0x40, 0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00};
        uint8_t tick = 0;
        uint8_t while_ = 0;

        while (true) {
            if (smartconfig_level_ == LedLevel::LEVEL_0) {
                if (setup) {
                    setup = false;
                    while_ = 0;

                    clean();
                    set_cursor(0, 113);
                    data_tran(close_eyes, sizeof(close_eyes));
                    render_text(0, 2, 0, (uint8_t *)"BPM :       bpm");
                    render_text(0, 4, 0, (uint8_t *)"SPO2:       \%");
                    render_text(0, 6, 19, (uint8_t *)"Huynh Dai Nghia");
                    render_text(0, 7, 22, (uint8_t *)"Tran Vinh Phat");
                }

                if (is_net_connected_) {
                    if (tick++ > 5) {
                        tick = 0;
                        set_cursor(0, 113);
                        data_tran(close_eyes, sizeof(close_eyes));
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        set_cursor(0, 113);
                        data_tran(open_eyes, sizeof(open_eyes));
                    }
                } else {
                    data_tran(close_eyes, sizeof(close_eyes));
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                get_time();
                snprintf(buffer, 11, "%04d-%02d-%02d", year, mon, mday);
                render_text(0, 0, 0, (uint8_t *)buffer);
                snprintf(buffer, 6, "%02d:%02d", hour, min);
                render_text(0, 0, 71, (uint8_t *)buffer);

                if (MAX30102::is_new_val_1()) {
                    snprintf(buffer, 16, "%5d", MAX30102::heart_rate_);
                    render_text(0, 2, 36, (uint8_t *)buffer);
                    snprintf(buffer, 16, "%3.2f", MAX30102::spo2_);
                    render_text(0, 4, 36, (uint8_t *)buffer);
                }

                vTaskDelay(750 / portTICK_PERIOD_MS);
            } else if (smartconfig_level_ == LedLevel::LEVEL_1) {
                if (setup) {
                    setup = false;
                    clean();
                    render_text(0, 2, 40, (uint8_t *)"Starting");
                    render_text(0, 3, 31, (uint8_t *)"smartconfig");
                    icon_loading(0);
                }
            } else if (smartconfig_level_ == LedLevel::LEVEL_2) {
                if (setup) {
                    setup = false;
                    clean();
                    render_text(0, 2, 49, (uint8_t *)"Enter");
                    render_text(0, 3, 37, (uint8_t *)"your WIFI");
                }
                else if (while_ == 0) icon_loading(1);
                else if (while_ == 1) icon_loading(2);
                else if (while_ == 2) icon_loading(3);
                else if (while_ == 3) icon_loading(4);
                else if (while_ == 4) icon_loading(5);
                else if (while_ >= 5) {
                    while_ = 0;
                    icon_loading(6);
                }
                while_++;
            } else if (smartconfig_level_ == LedLevel::LEVEL_3) {
                smartconfig_level_ = LedLevel::LEVEL_0;
                clean();

                if (is_net_connected_) render_text(0, 2, 43, (uint8_t *)"SUCCESS");
                else render_text(0, 2, 40, (uint8_t *)"CANCELED");
                vTaskDelay(3000 / portTICK_PERIOD_MS);
            }

            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        vTaskDelete(NULL);
    }

    void SH1106::set_cursor(uint8_t page, uint8_t col) {
        cmd_tran(0xB0 + page);
        cmd_tran(0x00 + ((col + 2) & 0x0F));
        cmd_tran(0x10 + (((col + 2) >> 4) & 0x0F));
    }

    void SH1106::cmd_tran(const uint8_t write_buf) {
        gpio_set_level(dc_pin_, 0);
        spi_driver_->write_bytes(dev_handle_, &write_buf, 1);
    }

    void SH1106::data_tran(const uint8_t *write_buf, size_t write_size) {
        gpio_set_level(dc_pin_, 1);
        spi_driver_->write_bytes(dev_handle_, write_buf, write_size);
    }

    void SH1106::config() {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        gpio_set_level(res_pin_, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        cmd_tran(DISP_OFF);

        cmd_tran(0x32); // Set Pump Volteage Value: 8 Vpp
        cmd_tran(0x40); // Set Display Start Line: 0
        cmd_tran(0x81); // Set Contrast Control Register:
        cmd_tran(0xFF);
        cmd_tran(0xA1); // Set Segment Re-map: Left rotate
        cmd_tran(0xA4); // Set Entire Display OFF/ON: OFF
        cmd_tran(0xA6); // Set Normal/Reverse Display: Normal
        cmd_tran(0xA8); // Set Multiplex Ration:
        cmd_tran(0x3F);
        cmd_tran(0xAD); // Set DC-DC OFF/ON:
        cmd_tran(0x8B);
        cmd_tran(0xC8); // Set Common Output Scan Direction: COM [N-1] to COM 0
        cmd_tran(0xD3); // Set Display Offset:
        cmd_tran(0x00);
        cmd_tran(0xD5); // Set Display Clock Devide Ratio/Oscillator Frequence:
        cmd_tran(0x80);
        cmd_tran(0xD9); // Set Dis-charge/Pre-charge Period:
        cmd_tran(0x22);
        cmd_tran(0xDA); // Set Common Pads Hardware Configuration:
        cmd_tran(0x12);
        cmd_tran(0xDB); // Set VCOM Deselect Level:
        cmd_tran(0x35);

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

    void SH1106::render_text(uint8_t font, uint8_t page, uint8_t col, const uint8_t *data) {
        cmd_tran(0xB0 + page);
        cmd_tran(0x00 + ((col + 2) & 0x0F));
        cmd_tran(0x10 + (((col + 2) >> 4) & 0x0F));

        for (uint8_t i = 0; i < strlen((char *)data); i++) {
            const uint8_t *c = get_font(font, data[i]);
            data_tran(c, 6);
        }
    }

    void SH1106::render(const uint8_t *pic) {
        for (uint8_t i = 0; i < 8; i++) {
            cmd_tran(0xB0 + i);
            cmd_tran(0x02);
            cmd_tran(0x10);

            data_tran(&pic[i*128], 128);
        }
    }

    void SH1106::icon_loading(uint8_t icon_num) {
        set_cursor(5, 59);
        data_tran(icon_load[icon_num*2], 10);
        set_cursor(6, 59);
        data_tran(icon_load[(icon_num*2) + 1], 10);

        // const uint8_t *p1 = icon_load[icon_num*2];
        // const uint8_t *p2 = icon_load[(icon_num*2) + 1];
        // set_cursor(5, 59);
        // data_tran(p1, 10);
        // set_cursor(6, 59);
        // data_tran(p2, 10);
    }

    void SH1106::event_smartconfig_screen(void *data) {
        auto level = *static_cast<LedLevel *>(data);
        smartconfig_level_ = level;
        setup = true;
    }

    void SH1106::event_network_status(void *data) {
        is_net_connected_ = *static_cast<NetStatus *>(data) == NetStatus::GOT_IP;
    }

} // namespace devices
