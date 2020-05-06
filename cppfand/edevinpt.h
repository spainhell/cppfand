#pragma once

#include "constants.h"
#include "ededit.h"
//#include "edscreen.h"


bool MyGetEvent();
void CtrlShiftAlt();
void ScrollPress();
void DisplLL(WORD Flags);
void Wr(pstring s, pstring OrigS);
bool ScrollEvent();
bool ViewEvent();
bool HelpEvent();
bool My2GetEvent();

