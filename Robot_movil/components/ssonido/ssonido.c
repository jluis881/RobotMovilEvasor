#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "ssonido.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#define TRIG_GPIO GPIO_NUM_35
#define ECHO_GPIO GPIO_NUM_36
#define TIMEOUT_US 30000

static float last_distance = -1;

static float hcsr04_read_cm(void)
{
    gpio_set_level(TRIG_GPIO, 0);
    ets_delay_us(2);

    gpio_set_level(TRIG_GPIO, 1);
    ets_delay_us(10);
    gpio_set_level(TRIG_GPIO, 0);

    int64_t timeout = esp_timer_get_time() + TIMEOUT_US;
    while (gpio_get_level(ECHO_GPIO) == 0) {
        if (esp_timer_get_time() > timeout) return -1;
    }

    int64_t start = esp_timer_get_time();

    while (gpio_get_level(ECHO_GPIO) == 1) {
        if (esp_timer_get_time() > timeout) return -1;
    }

    int64_t duration = esp_timer_get_time() - start;
    return duration / 58.0f;
}

static void hcsr04_task(void *arg)
{
    while (1) {
        last_distance = hcsr04_read_cm();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void hcsr04_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << TRIG_GPIO),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io);

    io.pin_bit_mask = (1ULL << ECHO_GPIO);
    io.mode = GPIO_MODE_INPUT;
    gpio_config(&io);

    xTaskCreate(hcsr04_task, "hcsr04_task", 4096, NULL, 5, NULL);
}

float hcsr04_get_distance_cm(void)
{
    return last_distance;
}
