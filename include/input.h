#ifndef INPUT_H
#define INPUT_H

#include "driver/gpio.h"

#define ENCODER_CLK_PIN GPIO_NUM_25
#define ENCODER_DT_PIN  GPIO_NUM_26
#define ENCODER_SW_PIN  GPIO_NUM_33

void input_task(void *pvParameters);

#endif
