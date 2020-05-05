#pragma once
#include "ededit.h"
#include "rdfrml.h"

FrmlPtr RdFldNameFrmlT(char& FTyp);
void MyWrLLMsg(pstring s);
void MyRunError(pstring s, WORD n);
void HMsgExit(pstring s);
WORD FindChar(WORD& Num, char C, WORD Pos, WORD Len); // ASM
bool TestOptStr(char c);

bool FindString(WORD& I, WORD Len);
WORD FindUpcChar(char C, WORD Pos, WORD Len);
WORD FindOrdChar(char C, WORD Pos, WORD Len);
bool SEquOrder(pstring S1, pstring S2);
void SetColorOrd(ColorOrd CO, WORD First, WORD Last); // r136
WORD FindCtrl(WORD F, WORD L);

void SimplePrintHead();
void GetNum(WORD& NN); // definice v printtxt.cpp

