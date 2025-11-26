#include "ota.h"
#include "esp_log.h"

#include "esp_err.h"
#include "esp_ota_ops.h"
#include "core/event_manager.h"

static const char *TAG = "OTA core";

namespace core {
    auto &ev_ota = EventManager::instance();

    static void print_sha256 (const uint8_t *image_hash, const char *label);
    static bool diagnostic(void);
    static void ota_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
    static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client);
    static esp_err_t validate_image_header(esp_app_desc_t *new_app_info);

    OTA::OTA() {
        uint8_t sha_256[32] = { 0 };
        esp_partition_t partition;

        // get sha256 digest for the partition table
        partition.address   = ESP_PARTITION_TABLE_OFFSET;
        partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
        partition.type      = ESP_PARTITION_TYPE_DATA;
        esp_partition_get_sha256(&partition, sha_256);
        print_sha256(sha_256, "SHA-256 for the partition table: ");

        // get sha256 digest for bootloader
        partition.address   = ESP_BOOTLOADER_OFFSET;
        partition.size      = ESP_PARTITION_TABLE_OFFSET;
        partition.type      = ESP_PARTITION_TYPE_APP;
        esp_partition_get_sha256(&partition, sha_256);
        print_sha256(sha_256, "SHA-256 for bootloader: ");

        // get sha256 digest for running partition
        esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
        print_sha256(sha_256, "SHA-256 for current firmware: ");
    }

    void OTA::init() {
        const esp_partition_t *running = esp_ota_get_running_partition();
        ESP_LOGW(TAG, "Running partition: %s", running->label);

        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                // run diagnostic function ...
                bool diagnostic_is_ok = diagnostic();
                if (diagnostic_is_ok) {
                    ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                    esp_ota_mark_app_valid_cancel_rollback();
                } else {
                    ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                    esp_ota_mark_app_invalid_rollback_and_reboot();
                }
            }
        }

        ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler, NULL));
    }

    void OTA::start() {
        init();

        xTaskCreate([](void *arg) { static_cast<OTA *>(arg)->start_task(arg); },
            "Start OTA task", 1024 * 1, this, 3, NULL
        );
    }

    void OTA::start_task(void *pvParameters) {
        ev_ota.subscribe(EventID::OTA, [this](void *arg) { this->update(arg); });
        vTaskDelete(NULL);
    }

    void OTA::update(void *data) {
        auto url = *static_cast<std::string *>(data);
        ESP_LOGI(TAG, "Starting OTA with URL: %s", url.c_str());

        esp_err_t err;
        esp_err_t ota_finish_err = ESP_OK;
        esp_http_client_config_t config = {
            .url = url.c_str(),
            // .cert_pem = (char *)server_cert_pem_start,
            .timeout_ms = 3000,
            .buffer_size = 2048,
            .buffer_size_tx = 1024,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .keep_alive_enable = true,
        };

        esp_https_ota_config_t ota_config = {
            .http_config = &config,
            .http_client_init_cb = _http_client_init_cb,
        };

        esp_https_ota_handle_t https_ota_handle = NULL;
        err = esp_https_ota_begin(&ota_config, &https_ota_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
            return;
        }

        esp_app_desc_t app_desc = {};
        err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed");
            goto ota_end;
        }
        err = validate_image_header(&app_desc);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "image header verification failed");
            goto ota_end;
        }

        while (1) {
            err = esp_https_ota_perform(https_ota_handle);
            if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
                break;
            }

            const size_t len = esp_https_ota_get_image_len_read(https_ota_handle);
            ESP_LOGD(TAG, "Image bytes read: %d", len);
        }

        if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
            // the OTA image was not completely received and user can customise the response to this situation.
            ESP_LOGE(TAG, "Complete data was not received.");
        } else {
            ota_finish_err = esp_https_ota_finish(https_ota_handle);
            if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
                ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
            } else {
                if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                    ESP_LOGE(TAG, "Image validation failed, image is corrupted");
                }
                ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
                return;
            }
        }

    ota_end:
        esp_https_ota_abort(https_ota_handle);
        ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
        return;
    }

    static void print_sha256 (const uint8_t *image_hash, const char *label) {
        char hash_print[32 * 2 + 1];
        hash_print[32 * 2] = 0;
        for (int i = 0; i < 32; ++i) {
            sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
        }
        ESP_LOGI(TAG, "%s: %s", label, hash_print);
    }

    static bool diagnostic(void) {
        // Test new firmware ok before set ESP_OTA_IMG_VALID
        // ...
        return true;
    }

    static void ota_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data) {
        if (event_base == ESP_HTTPS_OTA_EVENT) {
            switch (event_id) {
                case ESP_HTTPS_OTA_START:
                    ESP_LOGI(TAG, "OTA started");
                    break;
                case ESP_HTTPS_OTA_CONNECTED:
                    ESP_LOGI(TAG, "Connected to server");
                    break;
                case ESP_HTTPS_OTA_GET_IMG_DESC:
                    ESP_LOGI(TAG, "Reading Image Description");
                    break;
                case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
                    ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
                    break;
                case ESP_HTTPS_OTA_VERIFY_CHIP_REVISION:
                    ESP_LOGI(TAG, "Verifying chip revision of new image: %d", *(esp_chip_id_t *)event_data);
                    break;
                case ESP_HTTPS_OTA_DECRYPT_CB:
                    ESP_LOGI(TAG, "Callback to decrypt function");
                    break;
                case ESP_HTTPS_OTA_WRITE_FLASH:
                    ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
                    break;
                case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
                    ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
                    break;
                case ESP_HTTPS_OTA_FINISH:
                    ESP_LOGI(TAG, "OTA finish");
                    break;
                case ESP_HTTPS_OTA_ABORT:
                    ESP_LOGI(TAG, "OTA abort");
                    break;
            }
        }
    }

    static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client) {
        esp_err_t err = ESP_OK;
        /* Uncomment to add custom headers to HTTP request */
        // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
        return err;
    }

    static esp_err_t validate_image_header(esp_app_desc_t *new_app_info) {
        if (new_app_info == NULL) {
            return ESP_ERR_INVALID_ARG;
        }

        const esp_partition_t *running = esp_ota_get_running_partition();
        esp_app_desc_t running_app_info;
        if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
            ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
        }

        // if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        //     ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        //     return ESP_FAIL;
        // }

        return ESP_OK;
    }

} // namespace core
