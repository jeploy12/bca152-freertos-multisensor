#include <stdio.h>
#include "rtos_objects.h"

QueueHandle_t sensor_queue = NULL;
EventGroupHandle_t system_event_group = NULL;
SemaphoreHandle_t serial_mutex = NULL;
SemaphoreHandle_t data_mutex = NULL;
DisplayMode current_display_mode = MODE_TEMP;
SystemState current_system_state = STATE_ACTIVE;
// `= {}` rather than `= {0}`: this project builds with -Werror=missing-field-
// initializers, and {0} leaves the remaining members uninitialised.
static SensorData latest_data = {};
static volatile bool data_valid = false;

void publish_sensor_data(const SensorData *data) {
    if (data_mutex != NULL) {
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        latest_data = *data;
        data_valid = true;
        xSemaphoreGive(data_mutex);
    }
    // The queue is retained as a wake-up signal for the original design, but
    // consumers read the snapshot above so no sample is ever stolen.
    if (sensor_queue != NULL) {
        xQueueSend(sensor_queue, data, 0);
    }
    xEventGroupSetBits(system_event_group, EVENT_DATA_BIT);
}

bool read_latest_data(SensorData *out, uint32_t timeout_ms) {
    if (out == NULL) return false;

    // Wait for a publication. clearOnExit consumes the bit so a waiting task
    // wakes exactly once per sample; callers that must not steal the
    // notification from another task use read_latest_snapshot() instead.
    EventBits_t bits = xEventGroupWaitBits(
        system_event_group,
        EVENT_DATA_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(timeout_ms));

    if ((bits & EVENT_DATA_BIT) == 0) return false;

    return read_latest_snapshot(out);
}

bool read_latest_snapshot(SensorData *out) {
    if (out == NULL) return false;

    if (data_mutex != NULL) {
        if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
        if (!data_valid) {
            xSemaphoreGive(data_mutex);
            return false;
        }
        *out = latest_data;
        xSemaphoreGive(data_mutex);
    } else {
        if (!data_valid) return false;
        *out = latest_data;
    }
    return true;
}

void rtos_objects_init(void) {
    sensor_queue = xQueueCreate(5, sizeof(SensorData));
    system_event_group = xEventGroupCreate();
    serial_mutex = xSemaphoreCreateMutex();
    data_mutex = xSemaphoreCreateMutex();
    xEventGroupSetBits(system_event_group, EVENT_ACTIVE_BIT);
}

void safe_log(const char *msg) {
    if (serial_mutex != NULL) {
        if (xSemaphoreTake(serial_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            printf("%s\n", msg);
            xSemaphoreGive(serial_mutex);
        }
    } else {
        printf("%s\n", msg);
    }
}
