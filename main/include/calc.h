#pragma once

#include "system_state.h"
#include <stdint.h>

// *********************************************************************//

void calculateRpm(SystemState *state);
float runPid(SystemState *state, float kp, float ki, float kd);