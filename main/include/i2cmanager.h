#pragma once

#include <stdint.h>
#include "esp_err.h"

// *********************************************************************//

// Seesaw Button Interface - I2C Pins
#define I2C_PORT                I2C_NUM_0
#define SEESAW_ADDR             0x49
#define I2C_MASTER_SDA_IO       21
#define I2C_MASTER_SCL_IO       22
#define I2C_MASTER_FREQ_HZ      100000
#define I2C_MASTER_TIMEOUT_MS   1500

// I2C communication with seesaw button module begins with master device sending:
//      - a write header (r/w bit set 0)
//      - 2 register bytes (module and status)
//      - zero or more data bytes

// Seesaw Button modules
#define SEESAW_STATUS_BASE      0x00
#define SEESAW_GPIO_BASE        0x01
#define SEESAW_ENCODER_BASE     0x11

// Seesaw registers
#define SEESAW_STATUS_VERSION   0x02    // status base
#define SEESAW_GPIO_DIRCLR_BULK 0x03    // GPIO base, clears bits to set direction to input
#define SEESAW_GPIO_BULK        0x04    // GPIO base, read/write bulk pin state (write sets output level for outputs, no effect on inputs)
#define SEESAW_GPIO_BULK_SET    0x05    // GPIO base, write-only, sets bits in bulk register (see BULK)
#define SEESAW_GPIO_INTENSET    0x08    // GPIO base, write-only, sets bits in interrupt enable register (enables interrupts for pins)
#define SEESAW_GPIO_PULLENSET   0x0B    // GPIO base, write-only, sets bits in pull-up enable register (enables pull-ups for pins)
#define SEESAW_ENCODER_INTENSET 0x01    // encoder base, set interrupt
// *********************************************************************//

void i2cInit(void);
void i2cScan(void);
uint32_t readU32BE(uint8_t regHigh, uint8_t regLow, esp_err_t *out_err);
esp_err_t writeU32BE(uint8_t regHigh, uint8_t regLow, uint32_t value);
esp_err_t i2cWriteRegister(uint8_t regHigh, uint8_t regLow,
                                  const uint8_t *data, size_t len);