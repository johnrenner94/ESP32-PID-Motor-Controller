#pragma once

#include <stdint.h>
#include <stddef.h>

// *********************************************************************//

// TFT DISPLAY SPI PINS
#define SPI_MOSI 23
#define SPI_SCLK 18 
#define SPI_CS 25
#define SPI_DC 33
#define SPI_RESET 26
#define SPI_LITE 27
#define NOT_USED -1

#define SPI_CLOCK_SPEED_HZ 10 * 1000 * 1000   // 10 MHz
#define SPI_MAX_TRANSFER_SIZE 32768   // 16 KB
#define SPI_COLOR_CHUNK_SIZE 256

// ST7735 display commands
#define SWRESET 0x01    // Software reset
#define SLPOUT 0x11     //  Sleep Out
#define COLMOD 0x3A     // Color Mode
#define COLMOD_16bit 0x05   // Color mode 16bit
#define DISPON 0x29     // Display On
#define RASET 0x2B      // Row area set
#define CASET 0x2A      // Col area set
#define RAMWR 0x2C      // Memory write
#define RAMRD 0x2E      // Memory read
#define MADCTL 0x36     // Memory access control
#define INVOFF 0x20     // Invert off
#define INVON 0x21      // Inv On

#define yOffset 25      // display y coordinate offset

// *********************************************************************//

void displayInit();
void hardReset();
void displayWriteCommand(uint8_t cmd);
void displayWriteData(const uint8_t *data, size_t len);