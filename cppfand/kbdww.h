#pragma once
#include "constants.h"

longint PushWrLLMsg(WORD N, bool WithESC);
longint PushW(WORD C1, WORD R1, WORD C2, WORD R2);
void PopW(longint pos);

void WrLLF10MsgLine(); // r251
void WrLLF10Msg(WORD N); // r283
WORD RunErrNr;
void RunError(WORD N);
