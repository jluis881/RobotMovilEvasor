#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdio.h"
#include "esp_log.h"
#include "ble.h"
#include "cmotor.h"
#include "ssonido.h"
#include "rgbled.h"

static const char *TAG = "MOTOR_PID";

void task_monitor(void *);

void app_main(void)
{
    ESP_LOGI(TAG, "Inicio control PID 2 motores con timeout BLE");

    ble_init();         //Inicializa BLE
    hcsr04_init();      //Inicializa sensor de ultrasonido
    dir_init();         //Inicializa dirección de motor
    pwm_init();         //Inicializa PWM de motor
    encoder_init();     //Inicializa encoder de motor
    rgbled_init();      //Inicializa led RGB

    configASSERT( xTaskCreate(control_task, "control_task", 4096, NULL, 6, NULL));  //control PI
    configASSERT( xTaskCreate(rgbled_task, "rgbled_task", 1024, NULL, 3, NULL));    //led RGB pin 48
    configASSERT( xTaskCreate(task_monitor, "task_monitor", 2048, NULL, 2, NULL)); 


}

void task_monitor(void *){
    const char *nombre = pcTaskGetName(NULL);
    char *buffer_list = malloc(2048);
    if(!buffer_list){ESP_LOGW(nombre,"Sin memoria para vTaskList");}
    char *buffer_runtime = malloc(2048);
    if(!buffer_list){ESP_LOGW(nombre,"Sin memoria para vTaskGetRunTimeStats");}
    TickType_t last = xTaskGetTickCount();
    while (1) {

        vTaskList(buffer_list);
        printf("\nName          State   Prio     Stack   Num\n%s\n", buffer_list);
        vTaskGetRunTimeStats(buffer_runtime);
        printf("\nTask            Time(us)        %%CPU\n%s\n", buffer_runtime);

        vTaskDelayUntil(&last,pdMS_TO_TICKS(1000));
    }
}