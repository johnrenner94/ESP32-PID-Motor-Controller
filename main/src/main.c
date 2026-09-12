#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "system_state.h"
#include "input.h"
#include "display.h"
#include "display_mgr.h"
#include "motor.h"
#include "i2cmanager.h"
#include "calc.h"
#include "app_tasks.h"
#include "ui.h"

static const char *TAG = "main";    // for logging

// *********************************************************************//

SystemState state = {0};
UiState ui = {0};
int encoderDelta = 0;
waveformBuffer waveBuffer = {0};

// *********************************************************************//

void massInit(void){

    i2cInit();

    displayInit();

    inputInit();

    motorInit();

    wheelEncoderInit();

    uiInit(&ui, &state);

    ESP_LOGI(TAG, "Initialization complete");
}

void app_main(void){
    

    massInit();

    state.steadyState = true;

    xTaskCreate(controlTask, "control", 4096, NULL, 5, NULL);
    xTaskCreate(inputTask, "input", 4096, NULL, 3, NULL);
    xTaskCreate(displayTask, "display", 8192, NULL, 2, NULL);
    
}