#pragma once

enum class EnableLog {
    SHOW_OFF = 0,
    SHOW_ON
};

#define ESP_WIFI_AP_SSID "Thiet bi deo ESP32"
#define ESP_WIFI_AP_PASSWD "bonsochin"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN 10
#define ESP_WIFI_STA_SSID "TUAN"
#define ESP_WIFI_STA_PASSWD "700@700@T"
// #define ESP_WIFI_STA_SSID "Nokia"
// #define ESP_WIFI_STA_PASSWD "bonsochin"
#define ESP_MAXIMUM_RETRY 5

#define LED_SMARTCONFIG GPIO_NUM_2
#define BUTTON_SMARTCONFIG GPIO_NUM_5
// #define BUTTON_SMARTCONFIG GPIO_NUM_4

#define I2C_BUS_0 I2C_NUM_0
#define I2C_BUS_1 I2C_NUM_1
#define SCL_PIN GPIO_NUM_21
#define SDA_PIN GPIO_NUM_22

#define MAX30102_INTR GPIO_NUM_23
#define MAX30102_ADDRESS 0x57
#define MAX30102_FREQ_HZ 100000

// #define MAX30102_INTR GPIO_NUM_23
#define MPU6050_ADDRESS 0x68
#define MPU6050_FREQ_HZ 100000

/* ----- MQTT config ----- */
    #define SERVER_ADDRESS "171.246.121.79"
    #define PORT 1883

    // Publisher
    #define TOPIC_PUB_1 "center/data_sensor"

    // Json Pub/Sub format:
    // {
    //     "spo2": 0.0,
    //     "bpm": 0.0
    // }
    #define BPM "bpm"
    #define SPO2 "spo2"

/* End MQTT config */
