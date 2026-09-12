#pragma once

#include "system_state.h"
#include <stdint.h>

// *********************************************************************//

// color codes
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF81F
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0
#define CYAN 0x07FF
#define MAGENTA 0xF81F

// *********************************************************************//

void displayDrawBox(uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1, uint16_t color);
void displayDrawPixel(uint16_t x, uint16_t y, uint16_t color);
void displayWriteString(uint16_t x, uint16_t y, const char *str,uint16_t fgColor, uint16_t bgColor,uint8_t scale);
void displayDrawBootScreen();
void displayDrawMainScreen();