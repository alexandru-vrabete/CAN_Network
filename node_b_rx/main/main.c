#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"

#define TAG             "node_b"

#define CAN_TX_GPIO     5
#define CAN_RX_GPIO     4

#define SENSOR_MSG_ID   0x123

void app_main(void)
{
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

    /*
     * Hardware acceptance filter: let only standard ID 0x123 through.
     * For an 11-bit standard frame the controller compares bits [31:21],
     * so the ID has to be shifted left by 21. A 0 bit in the mask means
     * "must match", a 1 bit means "don't care".
     */
    twai_filter_config_t f_config = {
        .acceptance_code = (SENSOR_MSG_ID << 21),
        .acceptance_mask = ~(0x7FF << 21),
        .single_filter = true,
    };

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "TWAI started, accepting only ID 0x%03X", SENSOR_MSG_ID);

    while (1) {
        twai_message_t message;
        esp_err_t result = twai_receive(&message, portMAX_DELAY);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "Receive failed: %s", esp_err_to_name(result));
            continue;
        }

        /* second check in software, in case the filter is ever relaxed */
        if (message.identifier != SENSOR_MSG_ID || message.data_length_code < 1) {
            continue;
        }

        uint8_t sensor_value = message.data[0];
        ESP_LOGI(TAG, "Received ID=0x%03X DLC=%d sensor=%d",
                 message.identifier, message.data_length_code, sensor_value);
    }
}
