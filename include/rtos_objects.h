#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#define EVENT_ACTIVE_BIT (1 << 0)
#define EVENT_MOTION_BIT (1 << 1)
#define EVENT_ALARM_BIT  (1 << 2)
#define EVENT_DATA_BIT    (1 << 3)

typedef enum {
    MODE_TEMP = 0,
    MODE_HUMIDITY,
    MODE_LIGHT,
    MODE_MOTION,
    MODE_MAX
} DisplayMode;

typedef enum {
    STATE_ACTIVE,
    STATE_INACTIVE
} SystemState;

typedef struct {
    float temperature;
    float humidity;
    int light_percent;
    bool motion_detected;
} SensorData;

extern QueueHandle_t sensor_queue;
extern EventGroupHandle_t system_event_group;
extern SemaphoreHandle_t serial_mutex;
extern SemaphoreHandle_t data_mutex;
extern DisplayMode current_display_mode;
extern SystemState current_system_state;

// SensorTask publishes the newest sample here and signals EVENT_DATA_BIT.
// Consumers read the shared snapshot instead of each taking an item off the
// queue, so the display and the alarm can never compete for the same sample.
void publish_sensor_data(const SensorData *data);
bool read_latest_data(SensorData *out, uint32_t timeout_ms);
bool read_latest_snapshot(SensorData *out);

void rtos_objects_init(void);
void safe_log(const char *msg);

#endif
