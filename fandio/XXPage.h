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
	unsigned short Off = 0;
	pstring LastIndex;
	int LastRecNr = 0;
	int Sum = 0;
	bool IsLeaf = false;
	int GreaterPage = 0;
	unsigned short NItems = 0;
	unsigned char A[XPageSize];

	void Reset(XWorkFile* OwnerXW);
	void PutN(int N); // ASM
	void PutDownPage(int DownPage); // ASM
	void PutMLX(unsigned char M, unsigned char L); // ASM
	void ClearRest(); // ASM
	void PageFull();
	void AddToLeaf(FileD* file_d, WRec* R, XKey* KD, void* record);
	void AddToUpper(XXPage* P, int DownPage);
};
