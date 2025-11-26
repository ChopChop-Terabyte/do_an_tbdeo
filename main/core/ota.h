#pragma once

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include <string>

namespace core {
    class OTA {
    public:
    OTA();

    void init();
    void start();

    void start_task(void *pvParameters);
    void update(void *data);

    private:

    }; // class OTA

} // namespace core
