#include "input.h"
#include "motor.h"
#include "i2cmanager.h"
#include <string.h>
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

// *********************************************************************//

static const char *TAG = "input";

static bool encoderInitialized = false;
static volatile bool inputFlag = false;

static int32_t lastEncoderPosition = 0;

InputButtons buttons = {0};
InputButtons prevButtons = {0};

// *********************************************************************//

bool resetFlag(void)
{
    if (inputFlag) {
        inputFlag = false;
        gpio_set_level(2, 0);
        return true;
    }
    return false;
}

static void IRAM_ATTR seesawIntIsr(void *arg)
{
    inputFlag = true;
    gpio_set_level(2, 1);
    
}

int32_t buttonEncoderGetTicks(void)
{
    esp_err_t err;
    uint32_t delta = readU32BE(SEESAW_ENCODER_BASE, SEESAW_ENCODER_DELTA, &err);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "readEncoderDelta failed: %s (%d)", esp_err_to_name(err), err);
        return 0;
    }
    return (int32_t)delta;
}

static uint32_t readGpioBulk(void)
{
    esp_err_t err;
    uint32_t bulk = readU32BE(SEESAW_GPIO_BASE, SEESAW_GPIO_BULK, &err);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "readGpioBulk failed: %s (%d)", esp_err_to_name(err), err);
        return 0;
    }
    return bulk;
}

static bool readButtonActiveLow(uint32_t bulkState, uint8_t pin)
{
    return ((bulkState >> pin) & 0x1) == 0;
}

void inputPoll(SystemState *state)
{
    if (!encoderInitialized) {
        return;
    }

    uint32_t bulk = readGpioBulk();

    InputButtons current = {0};

    current.upPressed = readButtonActiveLow(bulk, BUTTON_UP);
    current.downPressed = readButtonActiveLow(bulk, BUTTON_DOWN);
    current.leftPressed = readButtonActiveLow(bulk, BUTTON_LEFT);
    current.rightPressed = readButtonActiveLow(bulk, BUTTON_RIGHT);
    current.selectPressed = readButtonActiveLow(bulk, BUTTON_SELECT);

    current.upActiveEdge =
        (!prevButtons.upPressed && current.upPressed);
    current.downActiveEdge =
        (!prevButtons.downPressed && current.downPressed);
    current.leftActiveEdge =
        (!prevButtons.leftPressed && current.leftPressed);
    current.rightActiveEdge =
        (!prevButtons.rightPressed && current.rightPressed);
    current.selectActiveEdge =
        (!prevButtons.selectPressed && current.selectPressed);

    buttons = current;
    prevButtons.upPressed = current.upPressed;
    prevButtons.downPressed = current.downPressed;
    prevButtons.leftPressed = current.leftPressed;
    prevButtons.rightPressed = current.rightPressed;
    prevButtons.selectPressed = current.selectPressed;
}

 InputButtons inputGetButtons(void)
 {
    InputButtons snapshot = buttons;

    buttons.upActiveEdge = false;
    buttons.downActiveEdge = false;
    buttons.leftActiveEdge = false;
    buttons.rightActiveEdge = false;
    buttons.selectActiveEdge = false;

    return snapshot;
 }

// *********************************************************************//

static esp_err_t configureButtons(void)
{
    esp_err_t err;
    const int max_attempts = 5;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        err = writeU32BE(SEESAW_GPIO_BASE, SEESAW_GPIO_DIRCLR_BULK, BUTTON_MASK);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "DIRCLR attempt %d failed: %s (%d)", attempt + 1, esp_err_to_name(err), err);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(10));

        err = writeU32BE(SEESAW_GPIO_BASE, SEESAW_GPIO_BULK_SET, BUTTON_MASK);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "BULK_SET attempt %d failed: %s (%d)", attempt + 1, esp_err_to_name(err), err);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(10));

        err = writeU32BE(SEESAW_GPIO_BASE, SEESAW_GPIO_PULLENSET, BUTTON_MASK);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "PULLENSET attempt %d failed: %s (%d)", attempt + 1, esp_err_to_name(err), err);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(10));

        // enable GPIO interrupts for buttons
        err = writeU32BE(SEESAW_GPIO_BASE, SEESAW_GPIO_INTENSET, BUTTON_MASK);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "INTENSET attempt %d failed: %s (%d)", attempt + 1, esp_err_to_name(err), err);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(10));

        return ESP_OK;
    }

    return err;
}

static void inputInterruptGpioInit(void)
{
    gpio_config_t ioConf = {
        .pin_bit_mask = (1ULL << SEESAW_INTERRUPT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    ESP_ERROR_CHECK(gpio_config(&ioConf));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SEESAW_INTERRUPT_PIN, seesawIntIsr, NULL));
}

static void interruptLedInit(void){
    gpio_config_t ledConfig = {
        .pin_bit_mask = (1UL << 2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE  
    };

    gpio_config(&ledConfig);

}

void inputInit(void)
{

    ESP_LOGI(TAG, "Initializing input...");

    esp_err_t err;
    uint32_t version = readU32BE(SEESAW_STATUS_BASE, SEESAW_STATUS_VERSION, &err);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Seesaw version read failed: %s (%d)", esp_err_to_name(err), err);
        return;
    }

    uint16_t productId = (uint16_t)((version >> 16) & 0xFFFF);
    ESP_LOGI(TAG, "Seesaw version=0x%08" PRIX32 " product=%u", version, productId);

    err = configureButtons();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Seesaw configureButtons failed: %s (%d)", esp_err_to_name(err), err);
        return;
    }

    prevButtons.upPressed = readButtonActiveLow(readGpioBulk(), BUTTON_UP);
    prevButtons.downPressed = readButtonActiveLow(readGpioBulk(), BUTTON_DOWN);
    prevButtons.leftPressed = readButtonActiveLow(readGpioBulk(), BUTTON_LEFT);
    prevButtons.rightPressed = readButtonActiveLow(readGpioBulk(), BUTTON_RIGHT);
    prevButtons.selectPressed = readButtonActiveLow(readGpioBulk(), BUTTON_SELECT);

    lastEncoderPosition = (int32_t)readU32BE(SEESAW_ENCODER_BASE, SEESAW_ENCODER_POSITION, &err);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Encoder position read failed: %s (%d)", esp_err_to_name(err), err);
        return;
    }

    encoderInitialized = true;
    ESP_LOGI(TAG, "Input initialized; encoder=%ld", (long)lastEncoderPosition);

    inputInterruptGpioInit();
    interruptLedInit();

    uint8_t enable = 1;
    i2cWriteRegister(SEESAW_ENCODER_BASE, SEESAW_ENCODER_INTENSET, &enable, 16);

    encoderInitialized = true;
}
