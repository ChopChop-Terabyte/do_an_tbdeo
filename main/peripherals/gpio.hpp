#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

namespace peripherals {
    enum class Level {
        LOW = 0,
        HIGH
    };

    enum class LedLevel {
        LEVEL_0 = 0,
        LEVEL_1,
        LEVEL_2,
        LEVEL_3
    };

    class GPIO {
    public:
        void init();
        void start();

        void start_task(void* pvParameters);
        static void output_config(uint64_t gpio_output, gpio_mode_t mode, gpio_pullup_t pullup_en, gpio_pulldown_t pulldown_en, gpio_int_type_t intr_type);
        static void input_config(uint64_t gpio_input, gpio_mode_t mode, gpio_pullup_t pullup_en, gpio_pulldown_t pulldown_en, gpio_int_type_t intr_type);
        static void pwm_config(uint64_t gpio_input, gpio_mode_t mode, gpio_pullup_t pullup_en, gpio_pulldown_t pulldown_en, gpio_int_type_t intr_type);

        // GPIO functions
        static void smartconfig_led_level(LedLevel level);
        void smartconfig_button_trigger(void *pvParameters);

        void event_smartconfig_led(void *data);
        void event_buzzer(int data);

    private:
        TaskHandle_t led_task_ = nullptr;

    }; // class GPIO

} // namespace peripherals
