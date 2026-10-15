#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"

#define TAG             "node_a"

#define CAN_TX_GPIO     5
#define CAN_RX_GPIO     4

#define SENSOR_MSG_ID   0x123   /* standard 11-bit CAN ID */
#define SENSOR_MSG_DLC  2       /* bytes of payload */
#define TX_PERIOD_MS    500

void app_main(void)
{
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "TWAI started, transmitting on ID 0x%03X every %d ms",
             SENSOR_MSG_ID, TX_PERIOD_MS);

    /* simulated sensor value, counts 0..100 and wraps around */
    uint8_t sensor_value = 0;

    while (1) {
        twai_message_t message = {0};
        message.identifier = SENSOR_MSG_ID;
        message.data_length_code = SENSOR_MSG_DLC;
        message.data[0] = sensor_value;
        message.data[1] = 0xA5;     /* fixed marker byte, helps spot the frame */

        esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(1000));
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Sent ID=0x%03X DLC=%d sensor=%d",
                     SENSOR_MSG_ID, SENSOR_MSG_DLC, sensor_value);
        } else {
            ESP_LOGW(TAG, "Transmit failed: %s", esp_err_to_name(result));
        }

        sensor_value++;
        if (sensor_value > 100) {
            sensor_value = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(TX_PERIOD_MS));
    }
}
