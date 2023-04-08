#pragma once
#include <string>
#include "LongStr.h"
#include "pstring.h"

bool EquUpCase(pstring& S1, pstring& S2);
bool EquUpCase(std::string S1, std::string S2);
bool EquUpCase(const char* S, pstring& S1);

short CompLongStr(LongStr* S1, LongStr* S2);
short CompLongShortStr(LongStr* S1, pstring* S2);
short CompArea(void* A, void* B, short L);
short CompStr(pstring& S1, pstring& S2);
int CompStr(std::string& S1, std::string& S2);

unsigned short CompLexLongStr(LongStr* S1, LongStr* S2);
unsigned short CompLexLongShortStr(LongStr* S1, pstring& S2);
unsigned short CompLexStr(pstring& S1, pstring& S2);
unsigned short CompLexStr(const pstring& S1, const pstring& S2);
unsigned short CompLexStrings(const std::string& S1, const std::string& S2);