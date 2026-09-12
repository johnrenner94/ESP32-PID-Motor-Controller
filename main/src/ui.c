#include "ui.h"
#include "display.h"
#include <stdio.h>
#include "system_state.h"
#include "motor.h"

// ****************************************************************************** //


// ****************************************************************************** //

void waveformAddSample(waveformBuffer *buf, int measured, int command) {
    buf->measuredRpm[buf->writeIndex] = measured;
    buf->rpmCommand[buf->writeIndex] = command;

    buf->writeIndex++;

    if (buf->writeIndex >= WAVEFORM_BUFFER_SIZE) {
        buf->writeIndex = 0;
        buf->full = true;
    }
}

// ****************************************************************************** //

static void uiNextScreen(UiState *ui)
{
    ui->currentScreen = (ui->currentScreen + 1) % SCREEN_COUNT;
    ui->needsRedraw = true;
    ui->needsUpdate = true;
}

static void uiPrevScreen(UiState *ui)
{
    if (ui->currentScreen == 0) {
        ui->currentScreen = SCREEN_COUNT - 1;
    } else {
        ui->currentScreen--;
    }
    ui->needsRedraw = true;
    ui->needsUpdate = true;
}

// ****************************************************************************** //

static void uiHandleMainScreen(UiState *ui, SystemState *state, const InputButtons *buttons, int encoderDelta)
{
    if (encoderDelta != 0) {
        state->rpmPendingCommand += encoderDelta * 10;   // step size example
        if (state->rpmPendingCommand < 0) {
            state->rpmPendingCommand = 0;
        }
        if (state->rpmPendingCommand > MOTOR_MAX_RPM) {
            state->rpmPendingCommand = MOTOR_MAX_RPM;
        }
        ui->needsUpdate = true;
    }

    if (buttons->selectActiveEdge) {
        if (state->rpmCommand != state-> rpmPendingCommand){
            state->rpmCommand = state->rpmPendingCommand;
            state->newCommand = true;
            ui->needsUpdate = true;
        }
    }
}

static void uiHandlePlotScreen(UiState *ui, SystemState *state, const InputButtons *buttons, int encoderDelta)
{
    ui->needsUpdate = true;

    (void)ui;
    (void)state;
    (void)buttons;
    (void)encoderDelta;


}

static void uiHandlePidScreen(UiState *ui, SystemState *state, const InputButtons *buttons, int encoderDelta)
{
    if (buttons->upActiveEdge) {
        if (ui->selectedPidField == 0) {
            ui->selectedPidField = PID_FIELD_COUNT - 1;
        } else {
            ui->selectedPidField--;
        }
        ui->needsUpdate = true;
    }

    if (buttons->downActiveEdge) {
        ui->selectedPidField = (ui->selectedPidField + 1) % PID_FIELD_COUNT;
        ui->needsUpdate = true;
    }

    if (encoderDelta != 0) {
        switch (ui->selectedPidField) {
            case PID_FIELD_KP:
                state->kp += encoderDelta * 0.1f;
                if (state->kp < 0.0f) state->kp = 0.0f;
                break;

            case PID_FIELD_KI:
                state->ki += encoderDelta * 0.01f;
                if (state->ki < 0.0f) state->ki = 0.0f;
                break;

            case PID_FIELD_KD:
                state->kd += encoderDelta * 0.01f;
                if (state->kd < 0.0f) state->kd = 0.0f;
                break;

            default:
                break;
        }

        ui->needsUpdate = true;
    }
}

void uiHandleInput(UiState *ui, SystemState *state, const InputButtons *buttons, int encoderDelta)
{
    if (buttons->leftActiveEdge) {
        uiPrevScreen(ui);
        return;
    }

    if (buttons->rightActiveEdge) {
        uiNextScreen(ui);
        return;
    }

    switch (ui->currentScreen) {
        case SCREEN_MAIN:
            uiHandleMainScreen(ui, state, buttons, encoderDelta);
            break;

        case SCREEN_PLOT:
            uiHandlePlotScreen(ui, state, buttons, encoderDelta);
            break;

        case SCREEN_PID:
            uiHandlePidScreen(ui, state, buttons, encoderDelta);
            break;

        default:
            break;
    }
}

// ****************************************************************************** //

void drawMainScreen(const UiState *ui, const SystemState *state)
{
    // background
    displayDrawBox(0, 160, 0, 79, BLACK);

    // banner
    displayWriteString(2, 2, "MAIN", WHITE, BLACK, 1);


    // left side
    displayWriteString(2, 16, "MEAS:", WHITE, BLACK, 1);

    displayWriteString(2, 28, " NEW:", YELLOW, BLACK, 1);

    displayWriteString(2, 40, " SET:", CYAN, BLACK, 1);

    displayWriteString(2, 52, " PWM: ", WHITE, BLACK, 1);

    // Right Side
    displayWriteString(70, 16, " ERR:", WHITE, BLACK, 1);

    displayWriteString(70, 28, "  OS:", YELLOW, BLACK, 1);

    displayWriteString(70, 40, "RISE:", CYAN, BLACK, 1);

    displayWriteString(70, 52, "SETT:", CYAN, BLACK, 1);

    displayWriteString(70, 64, " DIR:", CYAN, BLACK, 1);

    displayWriteString(70, 76, "  SS:", CYAN, BLACK, 1);

    (void)ui;
}

void updateMainScreen(const UiState *ui, const SystemState *state)
{

    char line[32];

    // left
    snprintf(line, sizeof(line), "%4d", state->measuredRpm);
    displayWriteString(30, 16, line, WHITE, BLACK, 1);

    snprintf(line, sizeof(line), "%4d", state->rpmPendingCommand);
    displayWriteString(30, 28, line, YELLOW, BLACK, 1);

    snprintf(line, sizeof(line), "%4d", state->rpmCommand);
    displayWriteString(30, 40, line, CYAN, BLACK, 1);

    snprintf(line, sizeof(line), "%4d", state->pwmCommand);
    displayWriteString(30, 52, line, CYAN, BLACK, 1);

    
    // right
    snprintf(line, sizeof(line), "%4d", state->error);
    displayWriteString(104, 16, line, WHITE, BLACK, 1);

    if(state->steadyState){
        if (state->direction == 1){
            snprintf(line, sizeof(line), "%.2f", state->overshoot);
            displayWriteString(104, 28, line, YELLOW, BLACK, 1);
        } else {
            snprintf(line, sizeof(line), "%.2f", state->undershoot);
            displayWriteString(104, 28, line, YELLOW, BLACK, 1);
        }

        snprintf(line, sizeof(line), "%.3f", state->riseTime);
        displayWriteString(104, 40, line, CYAN, BLACK, 1);

        snprintf(line, sizeof(line), "%.3f", state->settleTime);
        displayWriteString(104, 52, line, CYAN, BLACK, 1);

        snprintf(line, sizeof(line), "%4d", state->direction);
        displayWriteString(104, 64, line, CYAN, BLACK, 1);
    }
    else {
        displayWriteString(104, 28, " ----", YELLOW, BLACK, 1);
        displayWriteString(104, 40, " ----", CYAN, BLACK, 1);
        displayWriteString(104, 52, " ----", CYAN, BLACK, 1);
        displayWriteString(104, 64, " ----", CYAN, BLACK, 1);
    }
    snprintf(line, sizeof(line), "%4d", state->steadyState);
    displayWriteString(104, 76, line, CYAN, BLACK, 1);

    (void)ui;
}

void drawPlotScreen(const UiState *ui, const SystemState *state, waveformBuffer *buffer)
{
    displayDrawBox(1, 160, 0, 80, BLACK);
    displayWriteString(2, 2, "PLOT", WHITE, BLACK, 1);

    for (int i = 0; i < WAVEFORM_BUFFER_SIZE; i++){
        // int x = i + WAVEFORM_WINDOW_X_MARGIN;
        displayDrawBox((i), (i), 10, 80, BLACK);

        displayDrawBox(i, i, (150 - buffer->measuredRpm[i]), (150 - buffer->measuredRpm[i-1]), WHITE);
        displayDrawBox(i, i, (150 - buffer->rpmCommand[i]), (150 - buffer->rpmCommand[i-1]), GREEN);

        //displayDrawPixel(i, (150 - buffer->measuredRpm[i]), WHITE);
        //displayDrawPixel((i), (150 - buffer->rpmCommand[i]), GREEN);
    }

    (void)ui;
    (void)state;
}

void updatePlotScreen(const UiState *ui, const SystemState *state, waveformBuffer *buffer)
{
    static int lastIndex = 0;
    
    for (int i = lastIndex; i < buffer->writeIndex; i++){
       // int x = WAVEFORM_WINDOW_X_MARGIN;
        displayDrawBox(i, buffer->writeIndex, 10, 80, BLACK);

        displayDrawBox(i, i, (150 - buffer->measuredRpm[i]), (150 - buffer->measuredRpm[i-1]), WHITE);
        displayDrawBox(i, i, (150 - buffer->rpmCommand[i]), (150 - buffer->rpmCommand[i-1]), GREEN);

        //displayDrawPixel((i), (150 - buffer->measuredRpm[i]), WHITE);
        //displayDrawPixel((i), (150 - buffer->rpmCommand[i]), GREEN);
    }

    displayDrawBox(buffer->writeIndex, buffer->writeIndex, 10, 80, WHITE);

    lastIndex = buffer->writeIndex;

    (void)ui;
    (void)state;
}

void drawPidScreen(const UiState *ui, const SystemState *state)
{
    displayDrawBox(0, 160, 0, 79, BLACK);

    displayWriteString(2, 2, "PID", WHITE, BLACK, 1);

    uint16_t kpColor = (ui->selectedPidField == PID_FIELD_KP) ? YELLOW : WHITE;
    uint16_t kiColor = (ui->selectedPidField == PID_FIELD_KI) ? YELLOW : WHITE;
    uint16_t kdColor = (ui->selectedPidField == PID_FIELD_KD) ? YELLOW : WHITE;

    displayWriteString(2, 16, "KP: ", kpColor, BLACK, 1);

    displayWriteString(2, 28, "KI: ", kiColor, BLACK, 1);

    displayWriteString(2, 40, "KD: ", kdColor, BLACK, 1);
}

void updatePidScreen(const UiState *ui, const SystemState *state)
{

    char line[32];

    uint16_t kpColor = (ui->selectedPidField == PID_FIELD_KP) ? YELLOW : WHITE;
    uint16_t kiColor = (ui->selectedPidField == PID_FIELD_KI) ? YELLOW : WHITE;
    uint16_t kdColor = (ui->selectedPidField == PID_FIELD_KD) ? YELLOW : WHITE;

    snprintf(line, sizeof(line), "%.2f", state->kp);
    displayWriteString(26, 16, line, kpColor, BLACK, 1);

    snprintf(line, sizeof(line), "%.2f", state->ki);
    displayWriteString(26, 28, line, kiColor, BLACK, 1);

    snprintf(line, sizeof(line), "%.2f", state->kd);
    displayWriteString(26, 40, line, kdColor, BLACK, 1);
}

// ****************************************************************************** //

void uiRender(const UiState *ui, const SystemState *state, waveformBuffer *buffer)
{
    switch (ui->currentScreen) {
        case SCREEN_MAIN:
            drawMainScreen(ui, state);
            break;

        case SCREEN_PLOT:
            drawPlotScreen(ui, state, buffer);
            break;

        case SCREEN_PID:
            drawPidScreen(ui, state);
            break;

        default:
            break;
    }
}

void uiUpdate(const UiState *ui, const SystemState *state, waveformBuffer *buffer)
{
    switch (ui->currentScreen) {
        case SCREEN_MAIN:
            updateMainScreen(ui, state);
            break;

        case SCREEN_PLOT:
            updatePlotScreen(ui, state, buffer);
            break;

        case SCREEN_PID:
            updatePidScreen(ui, state);
            break;

        default:
            break;
    }
}

void uiInit(UiState *ui, SystemState *state)
{
    ui->currentScreen = SCREEN_MAIN;
    ui->selectedPidField = PID_FIELD_KP;
    ui->needsRedraw = true;
    ui->needsUpdate = true;

    state->rpmPendingCommand = 0;
    state->rpmCommand = 0;

    state->kp = 1.0f;
    state->ki = 0.01f;
    state->kd = 0.2f;
}