#pragma once
#include "constants.h"
#include "pstring.h"

using namespace std;

// ø. 142
void WriteWFrame(BYTE WFlags, string top, string bottom);
void WrHd(pstring s, string Hd, WORD Row, WORD MaxCols);
void CFileMsg(WORD n, char Typ); // ø. 279
void CFileError(WORD N); // r284

