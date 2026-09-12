#include "display.h"
#include "display_mgr.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "motor.h"
#include <stdlib.h>

static const char *TAG = "display";

const uint32_t upperFont[] = {

    0x3F48483F, 0x7F494936, 0X3E414141, 0x7F41413E, 0x7F494941, 0xFF484840,     // A - F
    0x3E414547, 0x7F08087F, 0x417F4100, 0x0641417E, 0x7F081463, 0x7F010101,     // G - L
    0x7F10207F, 0x7F300C7F, 0x3E41413E, 0x7F484830, 0x3E414D3E, 0x7F484C33,     // M - R
    0x31494946, 0x407F4040, 0x7F01017F, 0x700C037C, 0x7F02047F, 0x730C0C73,     // S - X
    0x7209097E, 0x63454973,                                                     // Y, Z
};

const uint32_t digitFont[] = {

    0x3E45493E,                              // 0
    0x01217F01, 0x33454931, 0x41494936,     // 1, 2, 3
    0x7808087F, 0x71494946, 0x3E494906,     // 4, 5, 6
    0x40474870, 0x36494936, 0x3048483F,     // 7, 8, 9
};

// *********************************************************************//

static void displayDefineWindow(uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1)
{
    int xMax;
    int xMin;
    int yMax;
    int yMin;

    if(x0 > x1){
        xMax = x0;
        xMin = x1;
    }
    else {
        xMax = x1;
        xMin = x0;
    }

    if(y0 > y1){
        yMax = y0;
        yMin = y1;
    }
    else {
        yMax = y1;
        yMin = y0;
    }



    displayWriteCommand(CASET);

    uint8_t colData[4] = {
        (xMin >> 8) & 0xFF,
        xMin & 0xFF,
        (xMax >> 8) & 0xFF,
        xMax & 0xFF
    };

    displayWriteData(colData, 4);

    displayWriteCommand(RASET);

    uint8_t rowData[4] = {
        (yMin >> 8) & 0xFF,
        yMin & 0xFF,
        (yMax >> 8) & 0xFF,
        yMax & 0xFF
    };
    displayWriteData(rowData, 4);

}

void displayDrawBox(uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1, uint16_t color){
    displayDefineWindow(x0, x1, y0 + yOffset, y1 + yOffset);
    displayWriteCommand(RAMWR);


    size_t totalPixels = (abs(x1 - x0) + 1) * (abs(y1 - y0) + 1);
    size_t totalBytes = totalPixels * 2; // 2 bytes per pixel for 16-bit color

    uint8_t colorChunk[SPI_COLOR_CHUNK_SIZE];
    for (size_t i = 0; i < SPI_COLOR_CHUNK_SIZE; i += 2){
        colorChunk[i] = (color >> 8) & 0xFF;
        colorChunk[i + 1] = color & 0xFF;
    }

    size_t fullChunk = totalBytes / SPI_COLOR_CHUNK_SIZE;
    size_t remainderChunk = totalBytes % SPI_COLOR_CHUNK_SIZE;

    for (size_t i = 0; i < fullChunk; i++){
        displayWriteData(colorChunk, SPI_COLOR_CHUNK_SIZE);
    }

    if (remainderChunk > 0){
        displayWriteData(colorChunk, remainderChunk);
    }
}

void displayDrawPixel(uint16_t x, uint16_t y, uint16_t color){
    displayDefineWindow(x, x, y + yOffset, y + yOffset);
    displayWriteCommand(RAMWR);
    uint8_t colorData[2] = {
        (color >> 8) & 0xFF,
        color & 0xFF
    };
    displayWriteData(colorData, 2);
} 

// *********************************************************************//

static uint32_t fontGetGlyph(char c) { 
    if (c == ' ') { 
        return 0x00000000; 
    } 
    
    if (c == '('){ 
        return 0x81C2241; 
    } 
        
    if (c == ')'){
        return 0x41221C08; 
    } 

    if (c == ':'){ 
        return 0x00666600; 
    } 

    if (c == '.'){
        return 0x00030300;
    }

    if (c == '-'){
        return 0x08080808;
    }

    if (c == '%'){
        return 0x220C3044;
    }

    if (c == '/'){
        return 0x020C3040;
    }

    if (c >= '0' && c <= '9') { 
        return digitFont[c - '0'];
    } 

    if (c >= 'A' && c <= 'Z') { 
        return upperFont[c - 'A'];
    } 

     return 0x552A552A; // blank for unsupported chars }
}

static void displayDrawChar(uint16_t x, uint16_t y, char c, uint16_t fgColor, uint16_t bgColor, uint8_t scale)
{

    uint32_t glyph = fontGetGlyph(c);

    for (uint8_t col = 0; col < 4; col++) {
        uint8_t columnBits = (glyph >> (8 * (3 - col))) & 0xFF;

        for (uint8_t row = 0; row < 7; row++) {
            bool pixelOn = (columnBits & (1 << (6 - row))) != 0;

            uint16_t color = pixelOn ? fgColor : bgColor;
            uint16_t x0 = x + col * scale;
            uint16_t x1 = x0 + scale - 1;
            uint16_t y0 = y + row * scale;
            uint16_t y1 = y0 + scale - 1;
            displayDrawBox(x0, x1, y0, y1, color);
        }
    }
}

void displayWriteString(uint16_t x, uint16_t y, const char *str,uint16_t fgColor, uint16_t bgColor,uint8_t scale)
{
    while (*str) {
  
        displayDrawChar(x, y, *str, fgColor, bgColor, scale);
        x += 6*scale;
        str++;
    }
}

// *********************************************************************//

void displayDrawBootScreen(){

    displayDrawBox(1, 160, 1, 80, WHITE);
    displayDrawBox(2, 159, 2, 79, BLACK); // background

    displayDrawBox(20, 40, 60, 78, CYAN); // red square

    displayDrawBox(41, 60, 60, 78, MAGENTA); // red square

    displayDrawBox(61, 80, 60, 78, YELLOW); // red square

    displayDrawBox(81, 100, 60, 78, RED); // red square

    displayDrawBox(101, 120, 60, 78, GREEN); // red square

    displayDrawBox(121, 140, 60, 78, BLUE); // red square

    displayWriteString(5, 5, "A B C D E F G H I", WHITE, BLACK, 1);
    displayWriteString(5, 15, "J K L M N O P Q R", WHITE, BLACK, 1);
    displayWriteString(5, 25, "S T U V W X Y Z", WHITE, BLACK, 1);
    displayWriteString(5, 35, "( ) : .", WHITE, BLACK, 1);
    displayWriteString(5, 45, "0 1 2 3 4 5 6 7 8 9", WHITE, BLACK, 1);

}

// void displayDrawMainScreen(){
//     displayDrawBox(2, 159, 2, 79, BLACK);
//     displayWriteString(5, 5, "NEW RPM: ", MAGENTA, BLACK, 1);
//     displayWriteString(5, 15, "SET RPM: ", WHITE, BLACK, 1);
//     displayWriteString(5, 25, "OBS RPM: ", WHITE, BLACK, 1);
//     displayWriteString(5, 35, "    PWM: ", WHITE, BLACK, 1);

//     char pwmMaxStr[8];
//     uint16_t pwmMax = MOTOR_PWM_MAX_DUTY;
//     snprintf(pwmMaxStr, sizeof(pwmMaxStr), "%d", pwmMax);

//     displayWriteString(91, 35, " / ", YELLOW, BLACK, 1);
//     displayWriteString(120, 35, pwmMaxStr, YELLOW, BLACK, 1);
// }