#pragma once

#include <vector>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "peripherals/gpio.hpp"
#include "peripherals/i2c.hpp"
#include "common/config.h"

#define MAX_CACHE_SIZE 200
#define FILTER_TIME 10000

#define PART_ID 0xFF

// Interrupt status
#define INTR_1 0x00                 // Interrupt status 1 register
#define INTR_2 0x01                 // Interrupt status 2 register

// Interrupt enable
#define INTR_EN_1 0x02              // Interrupt enable 1 register
#define INTR_EN_2 0x03              // Interrupt enable 2 register

// FIFO
#define FIFO_WRITE_PTR 0x04         // FIFO write pointer register
#define OVER_FLOW_COUNTER 0x05      // Over flow counter register
#define FIFO_READ_PTR 0x06          // FIFO read pointer register
#define FIFO_DATA 0x07              // FIFO data register

// FIFO configuration
#define FIFO_CONFIG 0x08            // FIFO configuration register

// Mode configuration
#define MODE_CONFIG 0x09            // Mode configuration register
#define MAX30102_RESET 0x40         // Need reconfig
#define MAX30102_SLEEP_ON 0x80      // Need reconfig
#define MAX30102_SLEEP_OFF 0x00     // Need reconfig
#define MAX30102_MULTI_LED 0x07

// SpO2 configuration
#define SPO2_SCALE_CONFIG 0x0A            // SpO2 configuration register

// LED Pulse amplitude
#define LED1_PA 0x0C                // LED 1 pulse amplitude register
#define LED2_PA 0x0D                // LED 2 pulse amplitude register

// Multi-LED mode control registers
#define SLOT_12 0x11                // Slot 1, 2 register
#define SLOT_34 0x12                // Slot 3, 4 register

// Temperature chip data
////

namespace devices {
    class MAX30102 {
    public:
        TaskHandle_t sensor_task = nullptr;
        static bool new_val;

        MAX30102(peripherals::I2CDriver *i2c_driver, i2c_port_num_t i2c_port_num, uint16_t device_address, uint32_t i2c_freq_hz,
                EnableLog show_values_log);
        ~MAX30102();

        void init();
        void start();

        void start_task(void *pvParameters);
        void config(uint8_t reg, uint8_t option);
        void config();
        uint8_t querry(uint8_t reg);
        void querry();
        void caculate();
        float get_spo2() { return spo2_; }
        int get_heart_rate() { return heart_rate_; }

        bool is_new_val() {
            if (new_val) {
                new_val = false;
                return true;
            }
            return false;
        }

    private:
        peripherals::I2CDriver *i2c_driver_;
        i2c_master_dev_handle_t dev_handle_;
        i2c_port_num_t i2c_port_num_;
        uint16_t device_address_;
        uint32_t i2c_freq_hz_;
        EnableLog show_values_log_;

        uint8_t trans_buf_[2];
        uint8_t recv_buf_;

        std::vector<uint32_t> ir_cache_;
        std::vector<uint32_t> red_cache_;
        float spo2_ = 0;        // %
        int heart_rate_ = 0;    // bpm

        // Timer
        esp_timer_handle_t start_timer_ = nullptr;
        bool timer_on_1_ = false;

        static void enable_timer_callback(void *pvParameters);

    }; // class MAX30102

} // namespace devices
