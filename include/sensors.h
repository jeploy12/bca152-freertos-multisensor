#ifndef SENSORS_H
#define SENSORS_H

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define DHT_PIN GPIO_NUM_15
#define LDR_CHANNEL ADC_CHANNEL_6

void sensor_task(void *pvParameters);

#endif
