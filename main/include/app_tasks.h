#pragma once

#include "system_state.h"
#include "ui.h"
#include "input.h"

// *********************************************************************//

#define CONTROL_LOOP_MS 20
#define DISPLAY_LOOP_MS 100
#define INPUT_LOOP_MS 10

// *********************************************************************//

void displayTask(void *pvParameters);
void controlTask(void *pvParameters);
void inputTask(void *pvParameters);