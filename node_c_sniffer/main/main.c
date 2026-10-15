#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"

#define CAN_TX_GPIO     5
#define CAN_RX_GPIO     4

/*
 * Listen-only sniffer node.
 *
 * In listen-only mode the controller never sends ACKs or error frames, so
 * it can sit on the bus without disturbing it. Every received frame is
 * printed on one line in a fixed format that the host script can parse:
 *
 *     ID: 0x123, DLC: 2, DATA: [0x12, 0x34]
 */
void app_main(void)
{
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_LISTEN_ONLY);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());

    while (1) {
        twai_message_t message;
        esp_err_t result = twai_receive(&message, portMAX_DELAY);
        if (result != ESP_OK) {
            continue;
        }

        printf("ID: 0x%03X, DLC: %d, DATA: [",
               message.identifier, message.data_length_code);
        for (int i = 0; i < message.data_length_code; i++) {
            printf("0x%02X", message.data[i]);
            if (i < message.data_length_code - 1) {
                printf(", ");
            }
        }
        printf("]\n");
    }
}
