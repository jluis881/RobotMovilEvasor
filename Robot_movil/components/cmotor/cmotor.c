#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pcnt.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "ble.h"
#include "ssonido.h"

#include "global.h"

// PINES
// Motor 1
#define PWM1_GPIO 18
#define DIR1_GPIO 11
#define ENCODER1_A_GPIO 4
#define ENCODER1_B_GPIO 5

// Motor 2
#define PWM2_GPIO 19
#define DIR2_GPIO 12
#define ENCODER2_A_GPIO 6
#define ENCODER2_B_GPIO 7

// PWM
#define PWM_FREQ 20000
#define PWM_RES LEDC_TIMER_8_BIT
#define PWM_MAX 250
#define PWM_MIN 75

// ENCODER
#define PULSES_PER_REV 186
#define SAMPLE_TIME_MS 50

// INGRESO DE DATO DE BLE
#define TIMEOUT_MS 100

static const char *TAG = "MOTOR_PID";

int64_t last_cmd_time = 0;

//volatile int comando = 0; //se comentó porque se utiliza como variable global para led rgb

// CONTROL (PI)
float kp = 1.45f;
float ki = 0.75f;
float target_rpm = 175.0f;

// integrales por motor
static float integral1 = 0;
static float integral2 = 0;

// slewrate por motor
float last_pwm1 = 0;
float last_pwm2 = 0;
float max_pwm_step = 4;

// INICIALIZACION PWM
void pwm_init(void)
{
    // Un timer para ambos PWM
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ,
        .duty_resolution = PWM_RES,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer);

    // Motor 1
    ledc_channel_config_t ch1 = {
        .gpio_num = PWM1_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ch1);

    // Motor 2
    ledc_channel_config_t ch2 = {
        .gpio_num = PWM2_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ch2);
}

void pwm_set(uint32_t duty, ledc_channel_t channel)
{
    if (duty > PWM_MAX)
        duty = PWM_MAX;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

// DIRECCION DE MOTOR
void dir_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DIR1_GPIO) | (1ULL << DIR2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

void set_direction(bool forward, int motor)
{
    if (motor == 1)
        gpio_set_level(DIR1_GPIO, forward ? 0 : 1); // Si forward es: true, gpio_s..(..,0); false, gpio_s..(..,1)
    else                                            // si no, motor 2
        gpio_set_level(DIR2_GPIO, forward ? 0 : 1);
}

// INICIALIZACION DE ENCODER
void encoder_init(void)
{
    // Motor 1 encoder
    pcnt_config_t pcnt_config1 = {
        .pulse_gpio_num = ENCODER1_A_GPIO,
        .ctrl_gpio_num = ENCODER1_B_GPIO,
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_UNIT_0,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .counter_h_lim = 32767,
        .counter_l_lim = -32768};
    pcnt_unit_config(&pcnt_config1);
    pcnt_set_filter_value(PCNT_UNIT_0, 100);
    pcnt_filter_enable(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);

    // Motor 2 encoder
    pcnt_config_t pcnt_config2 = {
        .pulse_gpio_num = ENCODER2_A_GPIO,
        .ctrl_gpio_num = ENCODER2_B_GPIO,
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_UNIT_1,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .counter_h_lim = 32767,
        .counter_l_lim = -32768};
    pcnt_unit_config(&pcnt_config2);
    pcnt_set_filter_value(PCNT_UNIT_1, 100);
    pcnt_filter_enable(PCNT_UNIT_1);
    pcnt_counter_clear(PCNT_UNIT_1);
    pcnt_counter_resume(PCNT_UNIT_1);
}

// CONTROL TASK
void control_task(void *arg)
{
    float max_integral = (PWM_MAX - PWM_MIN) / ki;
    int16_t pulses1 = 0, pulses2 = 0;
    char rx[20];

    while (1)
    {
        // Obtiene dato BLE
        if (ble_has_new_value())
        {
            ble_get_last_value(rx, sizeof(rx) - 1);
            comando = atoi(rx);
            ble_clear_new_value_flag();
            last_cmd_time = esp_timer_get_time();
            ESP_LOGI(TAG, "Comando BLE: %d", comando);
        }

        // TIMEOUT
        int64_t now = esp_timer_get_time();
        if ((now - last_cmd_time) > (TIMEOUT_MS * 1000))
        {
            comando = 0;
        }

        // ENCODERS
        pcnt_get_counter_value(PCNT_UNIT_0, &pulses1);
        pcnt_counter_clear(PCNT_UNIT_0);

        pcnt_get_counter_value(PCNT_UNIT_1, &pulses2);
        pcnt_counter_clear(PCNT_UNIT_1);

        float revs1 = (float)pulses1 / PULSES_PER_REV;
        float rpm1 = fabsf(revs1 * (60000.0f / SAMPLE_TIME_MS));

        float revs2 = (float)pulses2 / PULSES_PER_REV;
        float rpm2 = fabsf(revs2 * (60000.0f / SAMPLE_TIME_MS));

        /////////////////////////

        float distance = hcsr04_get_distance_cm();

        if (distance > 0 && distance <= 12.0f)
        {
            ESP_LOGW(TAG, "OBSTACULO %.1f cm", distance);
            comando=5;
            // FRENA
            pwm_set(0, LEDC_CHANNEL_0);
            pwm_set(0, LEDC_CHANNEL_1);
            vTaskDelay(pdMS_TO_TICKS(50));

            // GIRA EN SU LUGAR
            set_direction(false, 1); // motor izquierdo atrás
            set_direction(true, 2);  // motor derecho adelante

            pwm_set(190, LEDC_CHANNEL_0);
            pwm_set(190, LEDC_CHANNEL_1);
            vTaskDelay(pdMS_TO_TICKS(400));

            // 3. DETENERSE UN MOMENTO
            pwm_set(0, LEDC_CHANNEL_0);
            pwm_set(0, LEDC_CHANNEL_1);
            vTaskDelay(pdMS_TO_TICKS(100));

            continue; // vuelve a medir
        }

        ////////////////////////

        // CONTROL PI Motor 1
        float error1 = target_rpm - rpm1;
        integral1 += error1 * (SAMPLE_TIME_MS / 1000.0f);
        if (integral1 < 0)
            integral1 = 0;
        if (integral1 > max_integral)
            integral1 = max_integral;
        float u1 = kp * error1 + ki * integral1;
        float pwm1 = PWM_MIN + u1;
        last_pwm1 = pwm1;

        // CONTROL PI Motor 2
        float error2 = target_rpm - rpm2;
        integral2 += error2 * (SAMPLE_TIME_MS / 1000.0f);
        if (integral2 < 0)
            integral2 = 0;
        if (integral2 > max_integral)
            integral2 = max_integral;
        float u2 = kp * error2 + ki * integral2;
        float pwm2 = PWM_MIN + u2;
        last_pwm2 = pwm2;

        // ACCIONAMIENTO DE LOS MOTORES
        float pwm_left = pwm1;
        float pwm_right = pwm2;
        // float diff = 80;            // Para hacer diferencia de velocidad entre ruedas

        if (comando == 1)
        {
            set_direction(true, 1);
            set_direction(true, 2);
        }
        else if (comando == 3)
        {
            set_direction(false, 1);
            set_direction(false, 2);
        }
        else if (comando == 2)
        {
            set_direction(true, 1);
            set_direction(true, 2);
            // pwm_left  = pwm1 + diff;
            // pwm_right = pwm2 - diff;
            pwm_left = pwm1;
            pwm_right = 0;
        }
        else if (comando == 4)
        {
            set_direction(true, 1);
            set_direction(true, 2);
            // pwm_left  = pwm1 - diff;
            // pwm_right = pwm2 + diff;
            pwm_left = 0;
            pwm_right = pwm2;
        }
        else
        {
            pwm_left = 0;
            pwm_right = 0;
            integral1 = 0;
            integral2 = 0;
        }

        if (pwm_left < 0)
            pwm_left = PWM_MIN;
        if (pwm_left > PWM_MAX)
            pwm_left = PWM_MAX;
        if (pwm_right < 0)
            pwm_right = PWM_MIN;
        if (pwm_right > PWM_MAX)
            pwm_right = PWM_MAX;

        pwm_set((uint32_t)pwm_left, LEDC_CHANNEL_0);  // Motor 1
        pwm_set((uint32_t)pwm_right, LEDC_CHANNEL_1); // Motor 2

        ESP_LOGI(TAG,
                 "M1 Pulsos:%d RPM:%.1f PWM:%.0f | M2 Pulsos:%d RPM:%.1f PWM:%.0f CMD:%d",
                 pulses1, rpm1, pwm1, pulses2, rpm2, pwm2, comando);

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_TIME_MS));
    }
}