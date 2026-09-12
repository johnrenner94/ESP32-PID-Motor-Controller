#pragma once

#include <stdint.h>
#include "system_state.h"

// *********************************************************************//

#define MOTOR_PWM_GPIO      19
#define WHEEL_ENCODER_A_PIN 36
#define WHEEL_ENCODER_B_PIN 39

#define MOTOR_PWM_MODE      LEDC_LOW_SPEED_MODE
#define MOTOR_PWM_TIMER     LEDC_TIMER_0
#define MOTOR_PWM_CHANNEL   LEDC_CHANNEL_0
#define MOTOR_PWM_FREQ_HZ   20000
#define MOTOR_PWM_RES       LEDC_TIMER_10_BIT

#define MOTOR_PWM_MAX_DUTY  ((1 << 10) - 1)
#define MOTOR_PWM_MIN_DUTY 50

#define MOTOR_MAX_RPM   250

// *********************************************************************//

void motorInit(void);
void wheelEncoderInit(void);
void motorSetPwm(int pwm);
uint32_t wheelEncoderGetTicks(void);
int motorClampPwm(SystemState *state, int pwm);