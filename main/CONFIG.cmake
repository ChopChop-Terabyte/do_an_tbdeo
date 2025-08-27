set(SOURCES
    common/config.h

    core/event_manager.hpp
    core/event_manager.cpp

    peripherals/gpio.hpp
    peripherals/gpio.cpp
    peripherals/i2c.hpp
    peripherals/i2c.cpp
    network/net_manager.hpp
    network/net_manager.cpp
    network/wifi/wifi.cpp
    protocols/mqtt/mqtt.hpp
    protocols/mqtt/mqtt.cpp

    devices/max30102/max30102.hpp
    devices/max30102/max30102.cpp
    devices/mpu6050/mpu6050.hpp
    devices/mpu6050/mpu6050.cpp
)
