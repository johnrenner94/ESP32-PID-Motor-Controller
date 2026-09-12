#include "calc.h"
#include "motor.h"
#include "system_state.h"
#include "app_tasks.h"

// *********************************************************************//

int32_t lastTicks = 0;

const float dt = CONTROL_LOOP_MS / 1000.0f;
const float ticksPerRev = 1200.0f;
float filteredRpm = 0.0f;
const float alpha = 0.4f;   // for smoothing RPM signal,  

// *********************************************************************//

void calculateRpm(SystemState *state){

    int32_t nowTicks = wheelEncoderGetTicks();  // returns EncoderTicks, a count that never resets
    int32_t deltaTicks = nowTicks - lastTicks;
    lastTicks = nowTicks;
    state->measuredRpm = (deltaTicks * 60.0f) / (ticksPerRev * dt);
    filteredRpm = alpha * state->measuredRpm + (1.0f - alpha) * filteredRpm;
    state->measuredRpm = filteredRpm;
}

float runPid(SystemState *state, float kp, float ki, float kd){

    static float error = 0;
    static float lastError = 0;

    static float p_term = 0;
    static float i_term = 0;
    static float d_term = 0;

    error = state->rpmCommand - state->measuredRpm;
    state->error = error;

    p_term = error * kp;
    state->p_term = p_term;

    i_term = i_term + ki * lastError;
    state->i_term = i_term;

    d_term = kd * ((error - lastError) / dt);
    state->d_term = d_term;

    lastError = error;

    return (p_term + i_term + d_term);
}
