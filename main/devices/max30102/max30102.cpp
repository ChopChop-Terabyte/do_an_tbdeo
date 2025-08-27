#include "max30102.hpp"
#include "esp_log.h"

static const char *TAG = "MAX30102";

using namespace peripherals;

namespace devices {
    GPIO gpio;
    int64_t last_time_beat = 0;
    int64_t last_last_time_beat = 0;
    int64_t last_last_last_time_beat = 0;
    int64_t final_last_time_beat = 0;
    uint64_t last_beat = 0;
    uint64_t last_last_beat = 0;
    uint8_t i = 0;
    bool MAX30102::new_val = false;

    static void intr_handle(void *arg) {
        MAX30102 *self = static_cast<MAX30102*>(arg);
        BaseType_t hpw = pdFALSE;
        vTaskNotifyGiveFromISR(self->sensor_task, &hpw);
        portYIELD_FROM_ISR(hpw);
    }

    MAX30102::MAX30102(peripherals::I2CDriver *i2c_driver, i2c_port_num_t i2c_port_num, uint16_t device_address, uint32_t i2c_freq_hz,
                        EnableLog show_values_log)
                            : i2c_driver_(i2c_driver),
                            i2c_port_num_(i2c_port_num),
                            device_address_(device_address),
                            i2c_freq_hz_(i2c_freq_hz),
                            show_values_log_(show_values_log) {}

    MAX30102::~MAX30102() {
        if (sensor_task) {
            vTaskDelete(sensor_task);
            sensor_task = nullptr;
        }

        i2c_driver_->remove_dev(dev_handle_);
        ESP_LOGI(TAG, "MAX30102 deleted.");
    }

    void MAX30102::init() {
        i2c_driver_->add_dev(i2c_port_num_, &dev_handle_, device_address_, i2c_freq_hz_);
        ESP_LOGI(TAG, "MAX30102 Added");

        esp_timer_create_args_t log_timer_arg = {
            .callback = enable_timer_callback,
            .arg = this,
            .name = "MAX30102 timer",
        };
        esp_timer_create(&log_timer_arg, &start_timer_);
        esp_timer_start_periodic(start_timer_, 1000 * FILTER_TIME);

        while (true) {
            if (querry(PART_ID) == 0x15) {
                config();
                ESP_LOGI(TAG, "MAX30102 ready.");
                break;
            } else {
                ESP_LOGW(TAG, "Waitting MAX30102.");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }

        gpio.input_config(MAX30102_INTR, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_NEGEDGE);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(MAX30102_INTR, intr_handle, this);
    }

    void MAX30102::start() {
        init();

        xTaskCreatePinnedToCore([](void *arg) { static_cast<MAX30102 *>(arg)->start_task(arg); },
            "Start MAX30102 task", 1024 * 4, this, 2, &sensor_task, 1
        );
    }

    void MAX30102::start_task(void *pvParameters) {
        while (true) {
            if (ulTaskNotifyTake(pdTRUE, 500 / portTICK_PERIOD_MS) == pdTRUE) {
                uint8_t ret;
                ret = querry(INTR_1);
                if (ret & 0x40) querry();
                // else if (ret & 0x01) config();
            }
        }
    }

    void MAX30102::enable_timer_callback(void *pvParameters) {
        MAX30102 *arg = static_cast<MAX30102 *>(pvParameters);
        arg->timer_on_1_ = true;
    }

    void MAX30102::config(uint8_t reg, uint8_t option) {
        trans_buf_[0] = reg;
        trans_buf_[1] = option;
        i2c_driver_->write_bytes(dev_handle_, trans_buf_, 2);
    }

    void MAX30102::config() {
        config(MODE_CONFIG, MAX30102_RESET);
        querry(INTR_1);

        config(INTR_EN_1, 0x40);
        config(FIFO_CONFIG, 0x7F);
        config(MODE_CONFIG, MAX30102_MULTI_LED);
        config(SPO2_SCALE_CONFIG, 0x47);

        config(LED1_PA, 0x3F);         // LED 1 Pulse Amplitude register (Red led)
        config(LED2_PA, 0x3F);         // LED 2 Pulse Amplitude register (IR led)

        config(SLOT_12, 0x21);         /* Slot 1 RED led (SpO2 measurement) */   /* Slot 2 IR led (Heart rate measurement) */
        config(SLOT_34, 0x00);
        config(FIFO_WRITE_PTR, 0x00);
        config(OVER_FLOW_COUNTER, 0x00);
        config(FIFO_READ_PTR, 0x00);

        // ESP_LOGE(TAG, "0x%02X", querry(SPO2_SCALE_CONFIG));
    }

    uint8_t MAX30102::querry(uint8_t reg) {
        uint8_t data;
        trans_buf_[0] = reg;
        i2c_driver_->write_read(dev_handle_, trans_buf_, 1, &data, 1);

        return data;
    }

    void MAX30102::querry() {
        uint8_t data[6];
        trans_buf_[0] = FIFO_DATA;
        uint32_t ir_;
        uint32_t red_;

        // trans_buf_[1] = 0x05;
        // i2c_driver_->write_read(dev_handle_, &trans_buf_[1], 1, data, 2);
        // ESP_LOGW(TAG, "%d %d", data[0], data[1]);

        i2c_driver_->write_read(dev_handle_, trans_buf_, 1, data, 6);
        ir_ = ((data[0] & 0x03) << 16) | ((data[1] << 8) | data[2]);
        red_ = ((data[3] & 0x03) << 16) | ((data[4] << 8) | data[5]);

        // ESP_LOGW(TAG, "%ld", ir_);
        // ESP_LOGW(TAG, "%ld", red_);

        if (red_ >= 50000 && ir_ >= 50000) {
            ir_cache_.push_back(ir_);
            red_cache_.push_back(red_);
        } else {
            ir_cache_.clear();
            red_cache_.clear();

            last_time_beat = esp_timer_get_time();
            last_last_time_beat = 0;
            final_last_time_beat = 0;
            esp_timer_stop(start_timer_);
            esp_timer_start_periodic(start_timer_, 1000 * FILTER_TIME);
            timer_on_1_ = false;
        }

        if (timer_on_1_) {
            timer_on_1_ = false;

            caculate();

            ir_cache_.clear();
            red_cache_.clear();
            new_val = true;
        }
    }

    void MAX30102::caculate() {
        int filter = 3;
        uint64_t beat = 0;
        std::vector<float> spo2_cache;

        for (int i = filter; i < red_cache_.size() - filter; i++) {
            uint8_t var = 0;
            for (int j = -filter; j < filter + 1; j++) {
                if (red_cache_[i] > red_cache_[i+j]) {
                    var++;

                    spo2_cache.push_back( (float)ir_cache_[i] / (float)red_cache_[i] );
                }
            }
            if (var == filter * 2) beat++;
        }

        float sum = 0;
        for (int i = 0; i < spo2_cache.size(); i++) {
            sum += spo2_cache[i];
        }
        spo2_ = 100 * (sum / spo2_cache.size());

        if (!last_last_time_beat) heart_rate_ = beat * 60 * 1000000 / (esp_timer_get_time() - last_time_beat);
        else if (!final_last_time_beat) heart_rate_ = (beat + last_beat) * 60 * 1000000 / (esp_timer_get_time() - last_last_time_beat);
        else heart_rate_ = (beat + last_beat + last_last_beat) * 60 * 1000000 / (esp_timer_get_time() - final_last_time_beat);

        last_last_beat = last_beat;
        last_beat = beat;

        final_last_time_beat = last_last_time_beat;
        last_last_time_beat = last_time_beat;
        last_time_beat = esp_timer_get_time();

        if (show_values_log_ == EnableLog::SHOW_ON) {
            ESP_LOGW(TAG, "%f", get_spo2());
            ESP_LOGW(TAG, "%d", get_heart_rate());
        }
    }

} // namespace devices
