#pragma once
#include "constants.h"

using namespace std;

void AddBackSlash(string s);

// ø. 37
bool SEquUpcase(string s1, string s2);

// ø. 362
float Today();

// ø. 55
void StrLPCopy(string& Dest, string s, WORD MaxL);

// ø. 235
void SplitDate(float R, WORD& d, WORD& m, WORD& y);

//ø. 217
bool OlympYear(WORD year);

//ø. 219
WORD OlympYears(WORD year);
