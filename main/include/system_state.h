#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int pwmCommand;             // 0 to 1023
    int measuredRpm;         // placeholder for now
    int rpmCommand;
    int rpmPendingCommand;

    float kp;
    float ki;
    float kd;

    float p_term;
    float i_term;
    float d_term;

    int16_t error;
    float overshoot;
    float undershoot;
    float riseTime;
    float settleTime;
    float steadyStateError;
    float absoluteError;

    bool rising;
    bool direction;        // 1 for rising, 0 for falling
    bool newCommand;
    bool steadyState;
    bool enabled;            // future use
} SystemState;

typedef struct {
    
} metrics;