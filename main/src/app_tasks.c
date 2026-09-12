#include "app_tasks.h"
#include "display.h"
#include "input.h"
#include "calc.h"
#include "motor.h"
#include "system_state.h"
#include "ui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// *********************************************************************//

extern SystemState state;
extern UiState ui;
extern InputButtons buttons;
extern int encoderDelta;
extern waveformBuffer waveBuffer;

uint16_t maxRpm = 0;
uint16_t minRpm = 0;
uint16_t startSpeed;
uint16_t target;
uint16_t rpmChange;
uint16_t riseTarget;
uint16_t settleTargetLow;
uint16_t settleTargetHigh;
TickType_t commandTime;

// *********************************************************************//

void displayTask(void *pvParameters)
{
    
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1) {

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DISPLAY_LOOP_MS));

        if (ui.needsRedraw) {
            uiRender(&ui, &state, &waveBuffer);
            ui.needsRedraw = false;
        }

        if (ui.needsUpdate) {
            uiUpdate(&ui, &state, &waveBuffer);
            ui.needsUpdate = false;
        }

    }
}

void controlTask(void *pvParameters)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    static int16_t newPwm = 0;
    float correctionFactor = 0.0f;
    static int lastRpm = 0;
    static int k = 0;

    while (1) {

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(CONTROL_LOOP_MS));

        calculateRpm(&state);
        if (state.measuredRpm != lastRpm) ui.needsUpdate = true;
        lastRpm = state.measuredRpm;

        // init new command
        if (state.newCommand){
            commandTime = xTaskGetTickCount();
            
            startSpeed = state.measuredRpm;
            maxRpm = startSpeed;
            minRpm = startSpeed;
            target = state.rpmCommand;
            rpmChange = target - startSpeed;
            riseTarget = startSpeed + rpmChange *0.9f;
            settleTargetLow = target * 0.97f;
            settleTargetHigh = target * 1.03f; 
            state.overshoot = 0;
            state.undershoot = 0;
            state.riseTime = 0;
            state.settleTime = 0;
            state.steadyStateError = 0;
            state.absoluteError = 0;
            k = 0;

            if (target > startSpeed){
                state.direction = 1;
            } else {
                state.direction = 0;
            }

            state.newCommand = false;
            state.steadyState = false;
            state.rising = true;
        }

        if (!state.steadyState){

            // simple overshoot
            if(state.measuredRpm > maxRpm){
                maxRpm = state.measuredRpm;
            }

            if (state.measuredRpm < minRpm){
                minRpm = state.measuredRpm;
            }

            if (target > startSpeed){
                if (target != startSpeed) {
                    state.overshoot = 100 * (float)(maxRpm - target)/(float)(target - startSpeed);
                } 
            } else if (target < startSpeed) {
                if (target != startSpeed){
                    state.undershoot = 100 * (float)(target - minRpm)/(float)(startSpeed - target);
                }
            }
        
            // check rise time
            if (state.rising){
                bool reachedRiseTarget = false;

                if (state.direction == 1 && state.measuredRpm >= riseTarget){
                    reachedRiseTarget = true;
                }

                if (state.direction == 0 && state.measuredRpm <= riseTarget){
                    reachedRiseTarget = true;
                }

                if (reachedRiseTarget){
                    state.riseTime = pdTICKS_TO_MS((xTaskGetTickCount() - commandTime)) / 1000.0f;
                    ui.needsUpdate = true;
                    reachedRiseTarget = false;
                    state.rising = false;
                }
            }

            // settle time
            if ((state.measuredRpm >= settleTargetLow) && (state.measuredRpm <= settleTargetHigh)){
                if (!state.steadyState){
                    k++;
                    if (k == 100){
                        state.steadyState = true;
                        state.settleTime = pdTICKS_TO_MS((xTaskGetTickCount() - commandTime)) / 1000.0f;
                        state.steadyStateError = state.measuredRpm - target;
                        k = 0;
                        ui.needsUpdate = true;
                    }
                }
            } 
            else {
                state.steadyState = false;
                k = 0;
            }
        }
      

        correctionFactor = runPid(&state, state.kp, state.ki, state.kd);
        newPwm = newPwm + correctionFactor;

        newPwm = motorClampPwm(&state, newPwm);

        motorSetPwm(newPwm);

        state.pwmCommand = newPwm;

        waveformAddSample(&waveBuffer, state.measuredRpm, state.rpmCommand);
    }
}

void inputTask(void *pvParameters)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    static int8_t wheelEncoderDelta = 0;

    while (1) {

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(INPUT_LOOP_MS));

        wheelEncoderDelta = buttonEncoderGetTicks();

        if (resetFlag()) {
            inputPoll(&state);
        }

        InputButtons buttons = inputGetButtons();

        uiHandleInput(&ui, &state, &buttons, wheelEncoderDelta);
    }
}