#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "global.h"

#include "led_strip.h"
#define LED_GPIO 48
#define LED_NUM  1


static led_strip_handle_t strip;

void rgbled_init(){
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_NUM,
        //.led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(&strip_config, &rmt_config, &strip)
    );
}

void rgbled_task(void *arg){

    while(1) {
        if (comando == 0){
            led_strip_set_pixel(strip, 0, 255, 0, 0);
            led_strip_refresh(strip);
        }
        else if (comando == 5){
            led_strip_set_pixel(strip, 0, 0, 255, 0);
            led_strip_refresh(strip);
        }
        else {
            led_strip_set_pixel(strip, 0, 0, 0, 255);
            led_strip_refresh(strip);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}