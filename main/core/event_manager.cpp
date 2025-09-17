#include "event_manager.hpp"

namespace core {
    EventManager::EventManager(uint8_t queue_length) : queue_length_(queue_length),
                                                    task_handle_(nullptr),
                                                    running_(false) {}

    void EventManager::init() {
        queue_ = xQueueCreate(queue_length_, sizeof(mess_t));
        queue_intr_ = xQueueCreate(queue_length_, sizeof(mess_intr_t));
    }

    void EventManager::start() {
        if (running_) return;
        running_ = true;
        init();

        xTaskCreate([](void *arg) { static_cast<EventManager *>(arg)->start_task(); },
            "Process events task", 1024 * 5, this, 2, &task_handle_
        );
        xTaskCreate([](void *arg) { static_cast<EventManager *>(arg)->start_intr_task(); },
            "Process intr events task", 1024 * 2, this, 4, &task_handle_intr_
        );
    }

    void EventManager::stop() {
        running_ = false;
        if (task_handle_) {
            vTaskDelete(task_handle_);
            task_handle_ = nullptr;
        }
    }

    void EventManager::start_task() {
        mess_t ev;
        while (running_) {
            if (xQueueReceive(queue_, &ev, portMAX_DELAY) == pdTRUE) {
                std::lock_guard<std::mutex> lock(cb_mutex_);
                auto it = sub_list_.find(ev.id);
                if (it != sub_list_.end()) {
                    for (auto &cb : it->second) {
                        cb(ev.data);
                    }
                }
            }
        }
    }

    void EventManager::start_intr_task() {
        mess_intr_t ev;
        while (running_) {
            if (xQueueReceive(queue_intr_, &ev, portMAX_DELAY) == pdTRUE) {
                auto it = sub_list_intr_.find(ev.id);
                if (it != sub_list_intr_.end()) {
                    for (auto &cb : it->second) {
                        cb(ev.data);
                    }
                }
            }
        }
    }

    void EventManager::subscribe(EventID id, callback callback_ev) {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        sub_list_[id].emplace_back(callback_ev);
    }

    void EventManager::publish(EventID id, void *data) {
        if (!queue_) return;

        mess_t ev{id, data};
        xQueueSend(queue_, &ev, 0);
    }

    void EventManager::subscribe_intr(EventID id, intr_callback intr_callback_ev) {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        sub_list_intr_[id].emplace_back(intr_callback_ev);
    }

    void EventManager::publish_intr(EventID id, int data) {
        if (!queue_intr_) return;

        mess_intr_t ev{id, data};
        BaseType_t hpw = pdFALSE;
        xQueueSendFromISR(queue_intr_, &ev, &hpw);
        portYIELD_FROM_ISR(hpw);
    }

} // namespace core
