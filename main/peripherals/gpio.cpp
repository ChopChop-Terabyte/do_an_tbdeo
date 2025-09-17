#include "gpio.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "common/config.h"
#include "core/event_manager.hpp"

using namespace core;

namespace peripherals {
    auto &ev_gpio = EventManager::instance();
    static LedLevel led_level = LedLevel::LEVEL_0;
    static bool led_task_running_ = false;

    void GPIO::init() {
        output_config(BUZZER, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE);
        output_config(LED_SMARTCONFIG, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE);
        output_config(BUTTON_SMARTCONFIG, GPIO_MODE_INPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_ENABLE, GPIO_INTR_DISABLE);

        gpio_set_level(LED_SMARTCONFIG, 0);
        gpio_set_level(BUZZER, 0);
    }

    void GPIO::start() {
        init();

        xTaskCreate([](void *arg) { static_cast<GPIO *>(arg)->start_task(arg); },
            "Start GPIO task", 1024 * 5, this, 2, NULL
        );
    }

    void GPIO::start_task(void* pvParameters) {
        xTaskCreate([](void *arg) { static_cast<GPIO *>(arg)->smartconfig_button_trigger(arg); },
            "Start GPIO task", 1024, this, 2, NULL
        );

        ev_gpio.subscribe(EventID::LED_SC, [this](void *data) { this->event_smartconfig_led(data); });
        ev_gpio.subscribe_intr(EventID::BUZZER, [this](int data) { this->event_buzzer(data); });
        vTaskDelete(NULL);
    }

    void GPIO::output_config(uint64_t gpio_output, gpio_mode_t mode, gpio_pullup_t pullup_en, gpio_pulldown_t pulldown_en, gpio_int_type_t intr_type) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL<<gpio_output),
            .mode = mode,
            .pull_up_en = pullup_en,
            .pull_down_en = pulldown_en,
            .intr_type = intr_type,
        };
        gpio_config(&io_conf);
    }

    void GPIO::input_config(uint64_t gpio_input, gpio_mode_t mode, gpio_pullup_t pullup_en, gpio_pulldown_t pulldown_en, gpio_int_type_t intr_type) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL<<gpio_input),
            .mode = mode,
            .pull_up_en = pullup_en,
            .pull_down_en = pulldown_en,
            .intr_type = intr_type,
        };
        gpio_config(&io_conf);
    }

    void pwm_config(uint64_t pwm_pin, ledc_mode_t speed_mode, ledc_timer_bit_t duty_res, ledc_timer_t timer_num, uint32_t freq_hz) {
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = speed_mode,
            .duty_resolution  = duty_res,
            .timer_num        = timer_num,
            .freq_hz          = freq_hz,
            .clk_cfg          = LEDC_AUTO_CLK
        };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

        ledc_channel_config_t ledc_channel = {
            .gpio_num       = (int)pwm_pin,
            .speed_mode     = speed_mode,
            .channel        = LEDC_CHANNEL_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = timer_num,
            .duty           = 0,
            .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    void GPIO::smartconfig_led_level(LedLevel level) {
        uint32_t delay = 0;
        led_task_running_ = true;

        if (level == LedLevel::LEVEL_0) {
            gpio_set_level(LED_SMARTCONFIG, 0);
            led_task_running_ = false;
            vTaskDelete(NULL);
        } else if (level == LedLevel::LEVEL_1) delay = portMAX_DELAY;
        else if (level == LedLevel::LEVEL_2) delay = 500 / portTICK_PERIOD_MS;

        while (true) {
            gpio_set_level(LED_SMARTCONFIG, 1);
            vTaskDelay(delay);
            gpio_set_level(LED_SMARTCONFIG, 0);
            vTaskDelay(delay);
        }
    }

    void GPIO::smartconfig_button_trigger(void *pvParameters) {
        bool but_cur = gpio_get_level(BUTTON_SMARTCONFIG);

        while (true) {
            if (but_cur != gpio_get_level(BUTTON_SMARTCONFIG)) {
                but_cur = gpio_get_level(BUTTON_SMARTCONFIG);
                if (gpio_get_level(BUTTON_SMARTCONFIG)) ev_gpio.publish(EventID::BUTTON_SC);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }

    static void task_smartconfig_led(void *arg) {
        auto *task = static_cast<GPIO *>(arg);
        task->smartconfig_led_level(led_level);
    }

    void GPIO::event_smartconfig_led(void *data) {
        auto level = *static_cast<LedLevel *>(data);

        if (led_task_running_) {
            vTaskDelete(led_task_);
            led_task_ = nullptr;
            led_task_running_ = false;
        }
        led_level = level;

        xTaskCreate(task_smartconfig_led, "Start smartconfig led task", 1024, this, 2, &led_task_);
    }

    void GPIO::event_buzzer(int data) {
        gpio_set_level(BUZZER, data);
    }
} // namespace peripherals
