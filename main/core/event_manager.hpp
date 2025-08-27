#pragma once

#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace core {
    // #define MAX_SUBER 10;
    using callback = std::function<void (void *data)>;
    using intr_callback = std::function<void (int data)>;

    enum class EventID {
        BUTTON_SC = 0,
        LED_SC,
        NET_STATUS,
        MAX30102
    };

    typedef struct {
        EventID id;
        void *data;
    } mess_t;

    typedef struct {
        EventID id;
        int data;
    } mess_intr_t;

    class EventManager {
    public:
        static EventManager& instance() {
            static EventManager bus;
            return bus;
        }

        EventManager(const EventManager&) = delete;
        EventManager operator=(const EventManager&) = delete;

        void init();
        void start();
        void stop();

        void start_task();
        void start_intr_task();
        void subscribe(EventID id, callback callback_ev);
        void publish(EventID id, void *data = nullptr);
        void subscribe_intr(EventID id, intr_callback intr_callback_ev);
        void publish_intr(EventID id, int data = 0);

    private:
        EventManager(uint8_t queue_length = 1);

        uint8_t queue_length_;
        TaskHandle_t task_handle_;
        TaskHandle_t task_handle_intr_;
        QueueHandle_t queue_;
        QueueHandle_t queue_intr_;
        std::map<EventID, std::vector<callback>> sub_list_;
        std::map<EventID, std::vector<intr_callback>> sub_list_intr_;

        std::mutex cb_mutex_;
        bool running_;

    }; // class EventManager

} // namespace core