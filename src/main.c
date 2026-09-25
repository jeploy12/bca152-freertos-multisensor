#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#define DHT_PIN GPIO_NUM_15
#define LDR_CHANNEL ADC_CHANNEL_6 // GPIO 34 on ADC1

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

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    ets_delay_us(2000); 
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(30);

    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    if (wait_for_state(0, 80) < 0) return false;
    if (wait_for_state(1, 80) < 0) return false;
    if (wait_for_state(0, 80) < 0) return false;

    for (int i = 0; i < 40; i++) {
        if (wait_for_state(1, 60) < 0) return false;
        int duration = wait_for_state(0, 100);
        if (duration < 0) return false;

        data[i / 8] <<= 1;
        if (duration > 40) { 
            data[i / 8] |= 1;
        }
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return false;
    }

    int16_t t_hum = (data[0] << 8) | data[1];
    int16_t t_temp = (data[2] << 8) | data[3];

    *humidity = t_hum / 10.0f;
    *temperature = t_temp / 10.0f;

    return true;
}

void sensor_task(void *pvParameters) {
    float temp = 0.0f, hum = 0.0f;
    int raw_light = 0;
    int light_percent = 0;

    // Configure ADC1 using the updated ESP-IDF v5 OneShot API
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, LDR_CHANNEL, &config);

    // Initialize the baseline tick count for vTaskDelayUntil
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Read DHT22
        bool dht_success = read_dht22(&temp, &hum);

        // Read LDR and convert raw scale to 0-100%
        adc_oneshot_read(adc1_handle, LDR_CHANNEL, &raw_light);
        light_percent = (int)((raw_light / 4095.0f) * 100.0f);

        if (dht_success) {
            printf("Temp: %.1f C | Hum: %.1f %% | Light: %d %%\n", temp, hum, light_percent);
        } else {
            printf("DHT22 Error | Light: %d %%\n", light_percent);
        }
        fflush(stdout);

        // Unblock strictly every 2000 ticks relative to the last unblock time
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

void app_main() {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("Initializing Sensors...\n");
    fflush(stdout);

    xTaskCreate(sensor_task, "SensorTask", 2048, NULL, 2, NULL);
}