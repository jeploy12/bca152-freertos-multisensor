#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"
#include "system_state.h"

extern "C" void app_main(void) {
    rtos_objects_init();
    safe_log("[Main] FreeRTOS primitives initialized.");

    display_init();
    safe_log("[Main] SSD1306 Display initialized.");

    xTaskCreate(motion_task, "MotionTask", 2048, NULL, 3, NULL);
    xTaskCreate(input_task, "InputTask", 2048, NULL, 3, NULL);

    xTaskCreate(sensor_task, "SensorTask", 3072, NULL, 2, NULL);
    xTaskCreate(alarm_task, "AlarmTask", 2048, NULL, 2, NULL);
    xTaskCreate(state_task, "StateTask", 2048, NULL, 2, NULL);

    xTaskCreate(display_task, "DisplayTask", 3072, NULL, 1, NULL);

    safe_log("[Main] Scheduler started with all 6 mandatory tasks.");
}
