set(SOURCES
    common/config.h

    core/info.h
    core/event_manager.h
    core/event_manager.cpp
    core/sntp.h
    core/sntp.c
    core/ota.h
    core/ota.cpp

    peripherals/gpio.h
    peripherals/gpio.cpp
    peripherals/i2c.h
    peripherals/i2c.cpp
    peripherals/spi.h
    peripherals/spi.cpp
    network/net_manager.h
    network/net_manager.cpp
    network/wifi/wifi.cpp
    protocols/mqtt/mqtt.h
    protocols/mqtt/mqtt.cpp

    devices/max30102/max30102.h
    devices/max30102/max30102.cpp
    devices/mpu6050/mpu6050.h
    devices/mpu6050/mpu6050.cpp
    devices/oled/sh1106.h
    devices/oled/sh1106.cpp
)
