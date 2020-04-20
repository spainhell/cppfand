#pragma once
#include "access.h"
#include "constants.h"
#include "pstring.h"

struct RdbPos;
void Error(integer N); // r1
void SetInpLongStr(LongStrPtr S, bool ShowErr); // r37
void SetInpTTPos(longint Pos, bool Decode); // r46
void SetInpTT(RdbPos RP, bool FromTxt); // r53

void ReadChar(); // r73
WORD RdDirective(bool& b); // r81
void RdForwName(pstring& s); // r82
void SkipLevel(bool withElse); // r113

void SkipBlank(bool toNextLine); // r134
void RdBackSlashCode(); // r162
void RdLex(); // r179

bool IsForwPoint(); // r233

void Accept(char X); // r239

bool EquUpcase(const pstring& S); // r274 ASM