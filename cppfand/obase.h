#pragma once
#include "legacy.h"

extern bool SFlag, QFlag, WFlag, BFlag, DFlag, EFlag, AFlag, XFlag, VFlag, TFlag;
extern WORD CPState, CPCount;

void ResetCtrlFlags();
bool IsPrintCtrl(char C); // r43
void PrintByte(BYTE B);
void PrintByteStr(pstring S);
pstring CtrlToESC(char C);
WORD CPTest(char c); /* 0=binary;1=normal;2=control;3=bypass */
void TranslateCodePage(WORD& c);
void PrintChar(char C); // r123
void PrintStr(pstring s); // existuje jinde ...
WORD OpenLPTHandle();
bool ResetPrinter(WORD PgeLength, WORD LeftMargin, bool Adj, bool Frst); // r141
void ClosePrinter(WORD LeftMargin); // r169

extern bool PrintCtrlFlag;

void TestTxtHError(TextFile* F);
integer InputTxt(TextFile* F);
integer OutputTxt(TextFile* F);
integer OutputLPT1(TextFile* F);
integer FlushTxt(TextFile* F);
integer CloseTxt(TextFile* F);
integer CloseLPT1(TextFile* F);
integer OpenTxt(TextFile* F);
integer OpenLPT1(TextFile* F);
void Seek0Txt(TextFile* F); // r234
bool ResetTxt(TextFile* F);
bool RewriteTxt(TextFile* F, bool PrintCtrl);
void SetPrintTxtPath();

