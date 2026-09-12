#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "system_state.h"

// *********************************************************************//

// Seesaw ANO rotary nav encoder pins are bit indices in the seesaw GPIO register
#define BUTTON_LEFT         5
#define BUTTON_SELECT       1
#define BUTTON_RIGHT        3
#define BUTTON_DOWN         2
#define BUTTON_UP           4

#define SEESAW_ENCODER_POSITION 0x30    // encoder base, read-only, 32-bit big-endian current position
#define SEESAW_ENCODER_DELTA    0x40    // encoder base, read-only, 32-bit big-endian delta since last read of this register (or since encoder init if never read)
#define SEESAW_INTERRUPT_PIN 34

// Mask of all button bits in the GPIO register, used for configuring and reading button states, 0b001011010 or 0x2D
#define BUTTON_MASK ((1UL << BUTTON_UP) | (1UL << BUTTON_DOWN) | \
                     (1UL << BUTTON_LEFT) | (1UL << BUTTON_RIGHT) | \
                     (1UL << BUTTON_SELECT))

// *********************************************************************//

typedef struct {
    bool upPressed;
    bool downPressed;
    bool leftPressed;
    bool rightPressed;
    bool selectPressed;

    bool upActiveEdge;
    bool downActiveEdge;
    bool leftActiveEdge;
    bool rightActiveEdge;
    bool selectActiveEdge;
} InputButtons;

// *********************************************************************//

void inputInit(void);
void inputPoll(SystemState *state);
InputButtons inputGetButtons(void);
int32_t buttonEncoderGetTicks(void);

bool resetFlag(void);