#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtos_objects.h"
#include "system_state.h"

#define INACTIVITY_TIMEOUT_MS 15000

void state_task(void *pvParameters) {
    TickType_t last_motion_tick = xTaskGetTickCount();

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(system_event_group);

        if (bits & EVENT_MOTION_BIT) {
            // FR-09/FR-10: PIR motion is the ONLY authorised wake source.
            last_motion_tick = xTaskGetTickCount();
            if (current_system_state != STATE_ACTIVE) {
                current_system_state = STATE_ACTIVE;
                safe_log("[StateTask] Motion -> ACTIVE.");
            }
            xEventGroupSetBits(system_event_group, EVENT_ACTIVE_BIT);
        } else if (current_system_state == STATE_ACTIVE) {
            // Measure real elapsed time instead of assuming an exact loop period.
            TickType_t elapsed = xTaskGetTickCount() - last_motion_tick;
            if (elapsed >= pdMS_TO_TICKS(INACTIVITY_TIMEOUT_MS)) {
                current_system_state = STATE_INACTIVE;
                xEventGroupClearBits(system_event_group, EVENT_ACTIVE_BIT);
                safe_log("[StateTask] No motion for 15s -> INACTIVE.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
