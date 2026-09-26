#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rtos_objects.h"
#include "motion.h"

void motion_task(void *pvParameters) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Debounce state: a reading must be stable before it is believed.
    int stable_level = gpio_get_level(PIR_PIN);
    int candidate_level = stable_level;
    int stable_count = 0;

    for (;;) {
        int level = gpio_get_level(PIR_PIN);

        if (level == candidate_level) {
            // Candidate is holding; promote to stable once it persists.
            if (++stable_count >= PIR_DEBOUNCE_SAMPLES) {
                stable_level = candidate_level;
            }
        } else {
            // New edge, restart the confirmation window.
            candidate_level = level;
            stable_count = 0;
        }

        if (stable_level) {
            // Only the MOTION bit is published here. state_task is the single
            // owner of EVENT_ACTIVE_BIT (FR-09/FR-10: PIR is the sole wake source),
            // so writing ACTIVE from two tasks can no longer race the timeout clear.
            xEventGroupSetBits(system_event_group, EVENT_MOTION_BIT);
        } else {
            xEventGroupClearBits(system_event_group, EVENT_MOTION_BIT);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
