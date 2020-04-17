#pragma once

#include "ededit.h"
#include "edscreen.h"

using namespace std;

//const int COL = 80;
string OrigS = "    ";
WORD ww;

typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int longint;


bool MyGetEvent();
void CtrlShiftAlt();
void ScrollPress();
void DisplLL(WORD Flags);
void Wr(string s);
bool ScrollEvent();
bool ViewEvent();
bool HelpEvent();

