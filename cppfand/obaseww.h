#pragma once
#include "constants.h"
#include "pstring.h"


void* PushScr(WORD C1, WORD R1, WORD C2, WORD R2); // r72
longint PushW1(WORD C1, WORD R1, WORD C2, WORD R2, bool PushPixel, bool WW); // r80

// ø. 142
void WriteWFrame(BYTE WFlags, pstring top, pstring bottom);
void WrHd(pstring s, pstring Hd, WORD Row, WORD MaxCols);
longint PushWFramed(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, 
	pstring top, pstring bottom, BYTE WFlags); // r176
void CFileMsg(WORD n, char Typ); // ø. 279
void CFileError(WORD N); // r284

