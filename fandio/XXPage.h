#pragma once
#include "WRec.h"
#include "XKey.h"
#include "XPage.h"

class XWorkFile;

class XXPage /* for building XPage */
{
public:
	XXPage* Chain = nullptr;
	XWorkFile* XW = nullptr;
	WORD Off = 0;
	pstring LastIndex;
	longint LastRecNr = 0;
	longint Sum = 0;
	bool IsLeaf = false;
	longint GreaterPage = 0;
	WORD NItems = 0;
	BYTE A[XPageSize];

	void Reset(XWorkFile* OwnerXW);
	void PutN(longint N); // ASM
	void PutDownPage(longint DownPage); // ASM
	void PutMLX(BYTE M, BYTE L); // ASM
	void ClearRest(); // ASM
	void PageFull();
	void AddToLeaf(WRec* R, XKey* KD);
	void AddToUpper(XXPage* P, longint DownPage);
};
