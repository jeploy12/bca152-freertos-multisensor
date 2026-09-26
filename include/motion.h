#ifndef MOTION_H
#define MOTION_H

#include "driver/gpio.h"

#define PIR_PIN GPIO_NUM_27

// A reading must persist for this many consecutive polls (~200ms each) before
// it is accepted, which rejects single-sample glitches and noise spikes.
#define PIR_DEBOUNCE_SAMPLES 3

void motion_task(void *pvParameters);

#endif
