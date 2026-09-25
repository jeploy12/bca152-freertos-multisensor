#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    // Must end with \n to flush the line buffer
    printf("========================================\n");
    printf("[WOKWI] ESP32 Booted & Printing!\n");
    printf("========================================\n");

    int counter = 0;
    while (1) {
        printf("Heartbeat counter: %d\n", counter++);
        // Feeds the watchdog and yields CPU
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}