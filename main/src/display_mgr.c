#include "display_mgr.h"
#include "display.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// *********************************************************************//

static const char *TAG = "display_mgr";

spi_device_handle_t spiHandle;

// *********************************************************************//

static void displayWrite(const uint8_t *data, size_t len)
{
    spi_transaction_t t = {0};
    t.length = len * 8;
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_transmit(spiHandle, &t));
}

void displayWriteCommand(uint8_t cmd)
{
    gpio_set_level(SPI_DC, 0);
    displayWrite(&cmd, 1);
}

void displayWriteData(const uint8_t *data, size_t len)
{
    gpio_set_level(SPI_DC, 1);
    displayWrite(data, len);
}

static void displayBootCommandSequence(){
    displayWriteCommand(SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    displayWriteCommand(SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(150));

    displayWriteCommand(COLMOD);
    uint8_t colorMode = COLMOD_16bit;
    displayWriteData(&colorMode, 1);

    displayWriteCommand(INVON);

    displayWriteCommand(MADCTL);
    uint8_t madctl = 0xA8;
    displayWriteData(&madctl, 1);

    displayWriteCommand(DISPON);
    ESP_ERROR_CHECK(gpio_set_level(SPI_LITE, 1));
}

void displayHardReset()

{
    ESP_ERROR_CHECK(gpio_set_level(SPI_RESET, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(gpio_set_level(SPI_RESET, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void displayGpioInit()
{
    ESP_ERROR_CHECK(gpio_reset_pin(SPI_DC));
    ESP_ERROR_CHECK(gpio_set_direction(SPI_DC, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(SPI_DC, 0));

    ESP_ERROR_CHECK(gpio_reset_pin(SPI_RESET));
    ESP_ERROR_CHECK(gpio_set_direction(SPI_RESET, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(SPI_RESET, 1));

    ESP_ERROR_CHECK(gpio_reset_pin(SPI_LITE));
    ESP_ERROR_CHECK(gpio_set_direction(SPI_LITE, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(SPI_LITE, 0));
}

void displayInit(){

    ESP_LOGI(TAG, "initializing display...");

    spi_bus_config_t busConfig = {
        .mosi_io_num = SPI_MOSI,
        .sclk_io_num = SPI_SCLK,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
        .miso_io_num = NOT_USED,
        .quadwp_io_num = NOT_USED,
        .quadhd_io_num = NOT_USED,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devConfig = {
        .clock_speed_hz = SPI_CLOCK_SPEED_HZ,   // 10 MHz
        .mode = 0,                             // SPI mode 0
        .spics_io_num = SPI_CS,              // CS pin
        .queue_size =  1,
    };

   ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devConfig, &spiHandle));

    displayGpioInit();

    displayHardReset();

    displayBootCommandSequence();

    displayDrawBootScreen();

    vTaskDelay(pdMS_TO_TICKS(3000));

}