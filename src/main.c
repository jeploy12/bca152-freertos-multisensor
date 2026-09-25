#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#define DHT_PIN GPIO_NUM_15
#define LDR_CHANNEL ADC_CHANNEL_6 // GPIO 34 on ADC1

// 1. Define the Data Structure
typedef struct {
    float temperature;
    float humidity;
    int light_percent;
} SensorData;

// 2. Declare the Queue Handle globally
QueueHandle_t sensor_queue;

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
    SensorData current_data;

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

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Read sensors
        bool dht_success = read_dht22(&temp, &hum);
        adc_oneshot_read(adc1_handle, LDR_CHANNEL, &raw_light);
        
        // Populate the struct
        if (dht_success) {
            current_data.temperature = temp;
            current_data.humidity = hum;
        }
        current_data.light_percent = (int)((raw_light / 4095.0f) * 100.0f);

        // 3. Send data to the queue
        if (xQueueSend(sensor_queue, &current_data, pdMS_TO_TICKS(100)) != pdPASS) {
            printf("Queue full! Dropping data.\n");
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

// 4. Create a receiver task to verify the queue
void process_task(void *pvParameters) {
    SensorData received_data;
    
    for (;;) {
        // Wait indefinitely for new data to arrive in the queue
        if (xQueueReceive(sensor_queue, &received_data, portMAX_DELAY) == pdPASS) {
            printf("Queue Rx -> Temp: %.1f C | Hum: %.1f %% | Light: %d %%\n", 
                   received_data.temperature, received_data.humidity, received_data.light_percent);
            fflush(stdout);
        }
    }
}

void app_main() {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("Initializing System...\n");
    fflush(stdout);

    // 5. Initialize the Queue to hold up to 5 SensorData items
    sensor_queue = xQueueCreate(5, sizeof(SensorData));
    if (sensor_queue == NULL) {
        printf("Failed to create queue!\n");
        return;
    }

    // Launch both tasks
    xTaskCreate(sensor_task, "SensorTask", 2048, NULL, 2, NULL);
    xTaskCreate(process_task, "ProcessTask", 2048, NULL, 1, NULL);
}