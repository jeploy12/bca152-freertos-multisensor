#ifndef ALARM_H
#define ALARM_H

#include "driver/gpio.h"

#define ALARM_LED    GPIO_NUM_2
#define ALARM_BUZZER GPIO_NUM_18

typedef enum {
    ALARM_NORMAL,
    ALARM_LOW_TEMP,
    ALARM_HIGH_TEMP,
    ALARM_HIGH_HUM
} AlarmState;

AlarmState evaluate_temperature(float temp);
void alarm_task(void *pvParameters);

#endif
