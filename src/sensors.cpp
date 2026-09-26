#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "rtos_objects.h"
#include "sensors.h"

// Number of LDR conversions averaged per published sample, to suppress the
// read-to-read jitter that made the light percentage visibly flicker.
#define LDR_AVG_SAMPLES 8


static int wait_for_state(int state, int timeout_us) {
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(DHT_PIN) != state) {
        if ((esp_timer_get_time() - start) > timeout_us) return -1;
    }
    return (int)(esp_timer_get_time() - start);
}

static bool read_dht22(float *temp, float *hum) {
    uint8_t data[5] = {0};
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    ets_delay_us(2000);
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(30);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    if (wait_for_state(0, 80) < 0 || wait_for_state(1, 80) < 0 || wait_for_state(0, 80) < 0) return false;

    for (int i = 0; i < 40; i++) {
        if (wait_for_state(1, 60) < 0) return false;
        int duration = wait_for_state(0, 100);
        if (duration < 0) return false;
        data[i / 8] <<= 1;
        if (duration > 40) data[i / 8] |= 1;
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) return false;

    *hum = ((data[0] << 8) | data[1]) / 10.0f;
    *temp = ((data[2] << 8) | data[3]) / 10.0f;
    return true;
}

void sensor_task(void *pvParameters) {
    float temp = 25.0f, hum = 50.0f;
    SensorData current_data = {};

    adc_oneshot_unit_handle_t adc1_handle;
    
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {};
    config.bitwidth = ADC_BITWIDTH_DEFAULT;
    config.atten = ADC_ATTEN_DB_12;
    adc_oneshot_config_channel(adc1_handle, LDR_CHANNEL, &config);

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        // FR-10: "Sensors processed normally" is an ACTIVE-only behaviour. While
        // INACTIVE the DHT22 bit-bang read and ADC sampling are skipped entirely;
        // only the PIR (motion_task) stays alive so the system can wake.
        EventBits_t bits = xEventGroupGetBits(system_event_group);

        if (bits & EVENT_ACTIVE_BIT) {
            if (read_dht22(&temp, &hum)) {
                current_data.temperature = temp;
                current_data.humidity = hum;
            }
            // Average a burst of samples: a single raw LDR conversion is noisy
            // and made the displayed light percentage jitter between refreshes.
            long acc = 0;
            for (int i = 0; i < LDR_AVG_SAMPLES; i++) {
                int s = 0;
                adc_oneshot_read(adc1_handle, LDR_CHANNEL, &s);
                acc += s;
                vTaskDelay(pdMS_TO_TICKS(2));
            }
            current_data.light_percent = (int)(((float)(acc / LDR_AVG_SAMPLES) / 4095.0f) * 100.0f);
            current_data.motion_detected = (bits & EVENT_MOTION_BIT) != 0;

            // Publish the shared snapshot; consumers read it under data_mutex
            // instead of competing for queue items.
            publish_sensor_data(&current_data);
            safe_log("[SensorTask] Sampled telemetry published.");

            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
        } else {
            // Idle far more cheaply while INACTIVE.
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}