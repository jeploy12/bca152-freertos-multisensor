#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "rtos_objects.h"
#include "alarm.h"

AlarmState evaluate_temperature(float temp) {
    if (temp < 18.0f) return ALARM_LOW_TEMP;
    if (temp > 30.0f) return ALARM_HIGH_TEMP;
    return ALARM_NORMAL;
}

void alarm_task(void *pvParameters) {
    SensorData data;
    bool alarm_active = false;
    bool is_active = true;

    gpio_set_direction(ALARM_LED, GPIO_MODE_OUTPUT);

    // Timer configuration
    ledc_timer_config_t ledc_timer = {}; // Zero-initialize first
    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num        = LEDC_TIMER_0;
    ledc_timer.duty_resolution  = LEDC_TIMER_13_BIT;
    ledc_timer.freq_hz          = 4000;
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
    ledc_timer_config(&ledc_timer);

    // Channel configuration
    ledc_channel_config_t ledc_channel = {}; // Zero-initialize first
    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = LEDC_CHANNEL_0;
    ledc_channel.timer_sel      = LEDC_TIMER_0;
    ledc_channel.intr_type      = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num       = ALARM_BUZZER;
    ledc_channel.duty           = 0;
    ledc_channel.hpoint         = 0;
    ledc_channel_config(&ledc_channel);

    for (;;) {
        // Wait on the shared snapshot. The timeout keeps the task responsive to
        // an INACTIVE transition so the buzzer can be cut promptly, even though
        // SensorTask stops publishing while inactive.
        (void)read_latest_data(&data, 200);

        EventBits_t bits = xEventGroupGetBits(system_event_group);
        bool now_active = (bits & EVENT_ACTIVE_BIT) != 0;

        // Requirement: the alarm is a temperature-only check (LOW 18C / HIGH 30C).
        // The previous humidity > 80% condition was removed because it is not
        // part of the specification and could silence the buzzer for a reason
        // the system was never asked to watch.
        bool trigger_alarm = false;
        if (now_active) {
            trigger_alarm = (evaluate_temperature(data.temperature) != ALARM_NORMAL);
        }
        // FR-10: "Alarm active" is an ACTIVE-only behaviour. While INACTIVE the
        // buzzer stays silent even if temperature exceeds 30C.

        if (trigger_alarm != alarm_active || now_active != is_active) {
            alarm_active = trigger_alarm;
            is_active = now_active;
            if (trigger_alarm) {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 2048);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                safe_log("[AlarmTask] Alarm ACTIVE!");
            } else {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                safe_log(now_active ? "[AlarmTask] Alarm CLEARED."
                                    : "[AlarmTask] System INACTIVE - buzzer forced off.");
            }
        }

        gpio_set_level(ALARM_LED, trigger_alarm ? 1 : 0);

        if (trigger_alarm) {
            xEventGroupSetBits(system_event_group, EVENT_ALARM_BIT);
        } else {
            xEventGroupClearBits(system_event_group, EVENT_ALARM_BIT);
        }
    }
}
