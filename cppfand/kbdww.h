#pragma once
#include "constants.h"

void WrLLF10Msg(WORD N);
longint PushWrLLMsg(WORD N, bool WithESC);
longint PushW(WORD C1, WORD R1, WORD C2, WORD R2);
void PopW(longint pos);
