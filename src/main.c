#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#define DHT_PIN GPIO_NUM_15

static int wait_for_state(int state, int timeout_us) {
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(DHT_PIN) != state) {
        if ((esp_timer_get_time() - start_time) > timeout_us) {
            return -1;
        }
    }
    return (int)(esp_timer_get_time() - start_time);
}

static bool read_dht22(float *temperature, float *humidity) {
    uint8_t data[5] = {0};

    // Send Start Signal
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    ets_delay_us(2000); // 2ms low
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(30);

    // Prepare to Read Response
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    if (wait_for_state(0, 80) < 0) return false;
    if (wait_for_state(1, 80) < 0) return false;
    if (wait_for_state(0, 80) < 0) return false;

    // Read 40 Bits
    for (int i = 0; i < 40; i++) {
        if (wait_for_state(1, 60) < 0) return false;
        int duration = wait_for_state(0, 100);
        if (duration < 0) return false;

        data[i / 8] <<= 1;
        if (duration > 40) { // Safely between 28us (0) and 70us (1)
            data[i / 8] |= 1;
        }
    }

    // Verify Checksum
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return false;
    }

    // Convert Data
    int16_t t_hum = (data[0] << 8) | data[1];
    int16_t t_temp = (data[2] << 8) | data[3];

    *humidity = t_hum / 10.0f;
    *temperature = t_temp / 10.0f;

    return true;
}

void sensor_task(void *pvParameters) {
    float temp = 0.0f, hum = 0.0f;

    for (;;) {
        if (read_dht22(&temp, &hum)) {
            printf("Temperature: %.2f C | Humidity: %.2f %%\n", temp, hum);
        } else {
            printf("Failed to read from DHT22 sensor\n");
        }
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main() {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("Initializing DHT22...\n");
    fflush(stdout);

    xTaskCreate(sensor_task, "SensorTask", 2048, NULL, 2, NULL);
}