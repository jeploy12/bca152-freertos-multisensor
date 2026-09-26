#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rtos_objects.h"
#include "input.h"

void input_task(void *pvParameters) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENCODER_CLK_PIN) | (1ULL << ENCODER_DT_PIN) | (1ULL << ENCODER_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    int last_clk = gpio_get_level(ENCODER_CLK_PIN);

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(system_event_group);

        // FR-10: encoder is an ACTIVE-only behaviour. While INACTIVE, rotation
        // is ignored entirely and, unlike the PIR, must NOT wake the system.
        if (!(bits & EVENT_ACTIVE_BIT)) {
            // Keep the edge baseline current so a rotation spanning the wake
            // event is not misread as a single bogus step.
            last_clk = gpio_get_level(ENCODER_CLK_PIN);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        int current_clk = gpio_get_level(ENCODER_CLK_PIN);
        if (current_clk != last_clk && current_clk == 0) {
            if (gpio_get_level(ENCODER_DT_PIN) != current_clk) {
                current_display_mode = (DisplayMode)((current_display_mode + MODE_MAX - 1) % MODE_MAX);
            } else {
                current_display_mode = (DisplayMode)((current_display_mode + MODE_MAX - 1) % MODE_MAX);
            }
            safe_log("[InputTask] Rotary Encoder rotated -> Mode toggled.");
        }
        last_clk = current_clk;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
