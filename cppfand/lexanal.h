#pragma once
#include "access.h"
#include "constants.h"
#include "pstring.h"

struct RdbPos;


void Error(integer N); // r1
void SetInpStr(pstring& S); //r31
void SetInpLongStr(LongStr* S, bool ShowErr); // r37
void SetInpTTPos(longint Pos, bool Decode); // r46
void SetInpTT(RdbPos RP, bool FromTxt); // r53
void SetInpTTxtPos(FileDPtr FD);
void ReadChar(); // r73
WORD RdDirective(bool& b); // r81
void RdForwName(pstring& s); // r82
void SkipLevel(bool withElse); // r113
void SkipBlank(bool toNextLine); // r134
void OldError(integer N); // r167
void RdBackSlashCode(); // r170
void RdLex(); // r179
bool IsForwPoint(); // r233
void TestIdentif(); // r235
void TestLex(char X); // r237
void Accept(char X); // r239 ASM
integer RdInteger(); // r245
double RdRealConst(); // r250
double ValofS(pstring& S); // r251
bool EquUpcase(pstring& S); // r274 ASM

bool TestKeyWord(pstring S); // r282
bool IsKeyWord(pstring S); // r284 ASM
void AcceptKeyWord(pstring S); // r293
bool IsOpt(pstring S); // r296 ASM
bool IsDigitOpt(pstring S, WORD& N); // r305
pstring* RdStrConst(); // r314
char Rd1Char(); // r317
char RdQuotedChar(); // r320
bool IsIdentifStr(pstring& S); //r323

