#include "motor.h"
#include "system_state.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"

// *********************************************************************//

static const char *TAG = "motor";

volatile int32_t wheelEncoderTicks = 0;
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

// *********************************************************************//

uint32_t wheelEncoderGetTicks(void){
    uint32_t count;
    portENTER_CRITICAL(&encoderMux);
    count = wheelEncoderTicks;
    portEXIT_CRITICAL(&encoderMux);
    return count;
}

void motorSetPwm(int pwm){
    ESP_ERROR_CHECK(ledc_set_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL, pwm));
    ESP_ERROR_CHECK(ledc_update_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL));
}

int motorClampPwm(SystemState *state, int pwm){
    
        if (pwm > MOTOR_PWM_MAX_DUTY){
            pwm = MOTOR_PWM_MAX_DUTY;
        }

        if (pwm < 0){
            pwm = 0;
        }

        if ((state->rpmCommand == 0) & (state->measuredRpm == 0)){
            pwm = 0;
        }
    
        return pwm;
}

// ********************************************************************* //

static void IRAM_ATTR wheelEncoderISR(void *arg)
{
    gpio_num_t pin = (gpio_num_t)(uintptr_t)arg;

    int a = gpio_get_level(WHEEL_ENCODER_A_PIN);
    int b = gpio_get_level(WHEEL_ENCODER_B_PIN);

    portENTER_CRITICAL_ISR(&encoderMux);

    if (pin == WHEEL_ENCODER_A_PIN) {
        if (a == b) {
            wheelEncoderTicks--;
        } else {
            wheelEncoderTicks++;
        }
    } else if (pin == WHEEL_ENCODER_B_PIN) {
        if (a != b) {
            wheelEncoderTicks--;
        } else {
            wheelEncoderTicks++;
        }
    }

    portEXIT_CRITICAL_ISR(&encoderMux);
}

void motorInit(void)
{

    ESP_LOGI(TAG, "Initializing motor...");

    // config PWM timer and channel for motor control using LED Control peripheral
    ledc_timer_config_t timerConfig = {
        .speed_mode = MOTOR_PWM_MODE,
        .duty_resolution = MOTOR_PWM_RES,
        .timer_num = MOTOR_PWM_TIMER,
        .freq_hz = MOTOR_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timerConfig));

    ledc_channel_config_t channelConfig = {
        .gpio_num = MOTOR_PWM_GPIO,
        .speed_mode = MOTOR_PWM_MODE,
        .channel = MOTOR_PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channelConfig));

    ESP_LOGI(TAG, "PWM initialized on GPIO %d", MOTOR_PWM_GPIO);
}

void wheelEncoderInit(){
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WHEEL_ENCODER_A_PIN) | (1ULL << WHEEL_ENCODER_B_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_intr_type(WHEEL_ENCODER_A_PIN, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(WHEEL_ENCODER_B_PIN, GPIO_INTR_ANYEDGE);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(WHEEL_ENCODER_A_PIN, wheelEncoderISR, (void *)(uintptr_t)WHEEL_ENCODER_A_PIN);
     gpio_isr_handler_add(WHEEL_ENCODER_B_PIN, wheelEncoderISR, (void *)(uintptr_t)WHEEL_ENCODER_B_PIN);
}