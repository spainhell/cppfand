#pragma once
#include "constants.h"
#include "pstring.h"

class XItem // r274
{
public:
	XItem(BYTE* data, bool isLeaf);
	BYTE* Nr; // NN  RecNr /on leaf/ or NumberofRecordsBelow
	longint* DownPage; // not on leaf
	// M byte  number of equal bytes /not stored bytes/ 
	// Index string  /L=length, A area ptr/
	BYTE* XPageData;
	longint GetN();
	void PutN(longint N);
	WORD GetM(WORD O);
	void PutM(WORD O, WORD M);
	WORD GetL(WORD O);
	void PutL(WORD O, WORD L);
	XItem* Next(WORD O, bool isLeaf);
	WORD UpdStr(WORD O, pstring* S);
	size_t size(bool isLeaf); // vrati delku zaznamu
};
