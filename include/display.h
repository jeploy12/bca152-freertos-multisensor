#ifndef DISPLAY_H
#define DISPLAY_H

#include "driver/i2c.h"

#define OLED_ADDR 0x3C
#define I2C_MASTER_NUM I2C_NUM_0

void display_init(void);
void display_task(void *pvParameters);

#endif
