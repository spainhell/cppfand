#pragma once
#include "constants.h"

// r166
BYTE ConvKamenToCurr(char* Buf, WORD L);

// r333
void InitMouseEvents();

bool KbdTimer(WORD Delta, BYTE Kind); // r286
