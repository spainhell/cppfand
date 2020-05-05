#pragma once
#include "access.h"
#include "constants.h"
#include "pstring.h"

void MyMove(void* A1, void* A2, WORD N);
void ChainLast(void* Frst, void* New); // r13 ASM
void* LastInChain(void* Frst); // r18 ASM
WORD ListLength(void* P); // r22 ASM
void* ListAt(void* P, WORD I); // r26 ASM
void ReplaceChar(pstring S, char C1, char C2); // r30 ASM
bool SEquUpcase(pstring s1, pstring s2); // r37 ASM
pstring StrPas(const char* Src); // r49
void StrLPCopy(char* Dest, pstring s, WORD MaxL); // r55
WORD SLeadEqu(pstring S1, pstring S2); // r61 ASM
WORD EquMask1(); // r67 ASM
bool EqualsMask(void* p, WORD l, pstring Mask); // r86 ASM
bool EquLongStr(LongStrPtr S1, LongStrPtr S2); // ASM
bool EquArea(void* P1, void* P2, WORD L); // ASM
integer MinI(integer X, integer Y);
integer MaxI(integer X, integer Y);
WORD MinW(WORD X, WORD Y);
WORD MaxW(WORD X, WORD Y);
longint MinL(longint X, longint Y);
longint MaxL(longint X, longint Y);
longint SwapLong(longint N); // ASM
void ExChange(void* X, void* Y, WORD L); // ASM
bool OverlapByteStr(void* p1, void* p2); // ASM
WORD CountDLines(void* Buf, WORD L, char C); // r139 ASM
pstring GetDLine(void* Buf, WORD L, char C, WORD I); // r144 ASM
WORD FindCtrlM(LongStrPtr s, WORD i, WORD n); // r152
WORD SkipCtrlMJ(LongStrPtr s, WORD i); // r158
void AddBackSlash(pstring s); // r165
void DelBackSlash(pstring s); // r170
bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size); // r175 ASM
bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size); // r182 ASM - rozdìleno na txt a graph režim
longint HexStrToLong(pstring S); // ASM

// *** TIME - DATE
bool OlympYear(WORD year); // r217
WORD OlympYears(WORD year); // r219
double RDate(WORD Y, WORD M, WORD D, WORD hh, WORD mm, WORD ss, WORD tt); //
void SplitDate(double R, WORD& d, WORD& m, WORD& y); // r235
double AddMonth(double R, double RM);
double DifMonth(double R1, double R2);
void EncodeMask(pstring& Mask, WORD& Min, WORD& Max);
void AnalDateMask(const pstring& Mask, WORD& I, WORD& IDate, WORD& N);
bool ExCode(WORD N, const pstring& Mask);
double ValDate(const pstring& Txtpstring, pstring Mask); // r276
pstring StrDate(double R, pstring Mask); //r321
double Today(); // r362
double CurrTime();

// *** DISPLAY
WORD LenStyleStr(pstring s);
WORD LogToAbsLenStyleStr(pstring s, WORD l);
//bool SetStyleAttr(char c, BYTE& a);
void WrStyleChar(char c);
void WrStyleStr(pstring s, WORD Attr);
void WrLongStyleStr(LongStrPtr S, WORD Attr);
//void RectToPixel(WORD c1, WORD r1, WORD c2, WORD r2, WORD& x1, WORD& y1, WORD& x2, WORD& y2);

// *** DEBUGGING
#ifndef FandRunV
const char HexStr[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
pstring HexB(BYTE b);
pstring HexW(WORD i);
pstring HexD(longint i);
pstring HexPtr(void* p);
void DispH(void* ad, integer NoBytes);
#endif
