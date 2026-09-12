#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "system_state.h"
#include "input.h"

#define WAVEFORM_BUFFER_SIZE 160
#define WAVEFORM_X_MARGIN 0
#define WAVEFORM_PLOT_WIDTH (WAVEFORM_BUFFER_SIZE - WAVEFORM_X_MARGIN)

typedef enum {
    SCREEN_MAIN = 0,
    SCREEN_PLOT,
    SCREEN_PID,
    SCREEN_COUNT
} ScreenId;

typedef enum {
    PID_FIELD_KP = 0,
    PID_FIELD_KI,
    PID_FIELD_KD,
    PID_FIELD_COUNT
} PidField;

typedef struct {
    ScreenId currentScreen;
    PidField selectedPidField;
    bool needsRedraw;
    bool needsUpdate;
} UiState;

typedef struct {
    int measuredRpm[WAVEFORM_BUFFER_SIZE];
    int rpmCommand[WAVEFORM_BUFFER_SIZE];
    int writeIndex;
    bool full;
} waveformBuffer;

void uiInit(UiState *ui, SystemState *state);
void uiHandleInput(UiState *ui, SystemState *state, const InputButtons *buttons, int encoderDelta);
void uiRender(const UiState *ui, const SystemState *state, waveformBuffer *buffer);
void uiUpdate(const UiState *ui, const SystemState *state, waveformBuffer *buffer);
void waveformAddSample(waveformBuffer *buf, int measured, int command);