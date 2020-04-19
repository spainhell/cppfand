#pragma once
#include "constants.h"
#include "pstring.h"

using namespace std;

void AddBackSlash(pstring s);
void DelBackSlash(pstring s);

// r37
bool SEquUpcase(pstring s1, pstring s2);

bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size); // r175
bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size); // r182 - rozdìleno na txt a graph režim

// r362
double Today();

// r55
void StrLPCopy(pstring& Dest, pstring s, WORD MaxL);

// r235
void SplitDate(double R, WORD& d, WORD& m, WORD& y);

// r217
bool OlympYear(WORD year);

// r219
WORD OlympYears(WORD year);
