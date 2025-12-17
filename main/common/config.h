#pragma once

enum class EnableLog {
    SHOW_OFF = 0,
    SHOW_ON
};

#define ESP_WIFI_AP_SSID "Thiet bi deo ESP32"
#define ESP_WIFI_AP_PASSWD "bonsochin"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN 10
#define ESP_WIFI_STA_SSID "Quynh"
#define ESP_WIFI_STA_PASSWD "01886955488Qu@."
// #define ESP_WIFI_STA_SSID "SKYTECH"
// #define ESP_WIFI_STA_PASSWD "SKYTECH2025"
// #define ESP_WIFI_STA_SSID "Nokia"
// #define ESP_WIFI_STA_PASSWD "bonsochin"
// #define ESP_WIFI_STA_SSID "Pinvis"
// #define ESP_WIFI_STA_PASSWD "pipipipi"
#define ESP_MAXIMUM_RETRY 5

#define BUZZER_PIN GPIO_NUM_18
#define LED_SMARTCONFIG GPIO_NUM_2
#define BUTTON_SMARTCONFIG GPIO_NUM_5

// I2C
#define I2C_BUS_0 I2C_NUM_0
#define I2C_BUS_1 I2C_NUM_1
#define SCL_PIN GPIO_NUM_21
#define SDA_PIN GPIO_NUM_22

// SPI
#define SPI_HOST_0 SPI2_HOST
#define SPI_HOST_1 SPI3_HOST
#define SPI_CLK GPIO_NUM_14
#define SPI_MOSI GPIO_NUM_13
#define SPI_MISO GPIO_NUM_NC

// MAX30102
#define MAX30102_INTR GPIO_NUM_23
#define MAX30102_ADDRESS 0x57
#define MAX30102_FREQ_HZ 100000

// MPU6050
#define MPU6050_ADDRESS 0x68
#define MPU6050_FREQ_HZ 100000

// SH1106
#define SH1106_CS GPIO_NUM_15
#define SH1106_DC GPIO_NUM_27
#define SH1106_RES GPIO_NUM_12
#define SH1106_FREQ_HZ 4000000

/* ----- MQTT config ----- */
    #define SERVER_ADDRESS "nghiadev.ddns.net"
    #define PORT 1883

    // Publisher 1
    // {
    //     "ping": "1"
    // }
    #define TOPIC_CENTER_CONNECT "center/connect"
    // {
    //     "id": "000000",
    //     "name": "",
    //     "gender": "",
    //     "age": "0",
    //     "weight": "0.0",
    //     "height": "0.0"
    // }
    #define TOPIC_CENTER_INFO "center/data_info_1"
    // {
    //     "id": "000000",
    //     "bpm": "0",
    //     "spo2": "0.0"
    // }
    #define TOPIC_CENTER_SENSOR "center/data_sensor_1"

    // Subscriber 1
    // {
    //     "ping": "1"
    // }
    #define TOPIC_CLIENT_CONNECT "client/connect"
    // {
    //     "id": "000000",
    //     "name": "",
    //     "gender": "",
    //     "age": "0",
    //     "weight": "0.0",
    //     "height": "0.0"
    // }
    #define TOPIC_CLIENT_INFO "client/data_info_1"
    // {
    //     "notice": "0"
    // }
    #define TOPIC_CLIENT_NOTICE "client/notice"
    // {
    //     "url": "https://github.com/ChopChop-Terabyte/firmware_ota/releases/download/firdmware_esp32/tb_deo_error.bin"
    // }
    #define TOPIC_CLIENT_OTA "client/ota"

    #define PING "ping"
    #define ID "id"
    #define BPM "bpm"
    #define SPO2 "spo2"
    #define NAME "name"
    #define GENDER "gender"
    #define AGE "age"
    #define WEIGHT "weight"
    #define HEIGHT "height"
    #define NOTICE "notice"
    #define URL "url"

/* End MQTT config */
