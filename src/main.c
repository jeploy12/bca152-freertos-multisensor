#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_a(void *pvParameters) {
    for (;;) {
        printf("Task A running\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_b(void *pvParameters) {
    for (;;) {
        printf("Task B running\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void app_main() {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");
    fflush(stdout);

    xTaskCreate(task_a, "Task A", 2048, NULL, 1, NULL);
    xTaskCreate(task_b, "Task B", 2048, NULL, 1, NULL);
}