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
short InputTxt(TextFile* F);
short OutputTxt(TextFile* F);
short OutputLPT1(TextFile* F);
short FlushTxt(TextFile* F);
//short CloseTxt(TextFile* F);
short CloseLPT1(TextFile* F);
//short OpenTxt(TextFile* F);
short OpenLPT1(TextFile* F);
void Seek0Txt(TextFile* F); // r234
//bool ResetTxt(TextFile* F);
bool RewriteTxt(std::string path, TextFile* F, bool PrintCtrl);
void SetPrintTxtPath();

