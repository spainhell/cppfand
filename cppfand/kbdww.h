#pragma once
#include "constants.h"

longint PushWrLLMsg(WORD N, bool WithESC);
longint PushW(WORD C1, WORD R1, WORD C2, WORD R2);
void PopW(longint pos);
void PopScr(void* p); // r120
void PopW2(longint pos, bool draw); // r128

void WrLLMsg(WORD N); // r220
void WrLLMsgTxt(); // r234
void WrLLF10MsgLine(); // r251
void WrLLF10Msg(WORD N); // r283
bool PromptYN(WORD NMsg); // r286
WORD RunErrNr;
void RunError(WORD N);
