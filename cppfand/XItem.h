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
	longint GetN(); // index.pas r129 ASM
	void PutN(longint N); // index.pas r132 ASM
	WORD GetM(WORD O); // index.pas r136 ASM
	void PutM(WORD O, WORD M); // index.pas r139 ASM
	WORD GetL(WORD O); // index.pas r142 ASM
	void PutL(WORD O, WORD L); // index.pas r145 ASM
	XItem* Next(WORD O, bool isLeaf); // index.pas r148 ASM
	WORD UpdStr(WORD O, pstring* S); // index.pas r152 ASM
	size_t size(bool isLeaf); // vrati delku zaznamu
};
typedef XItem* XItemPtr;
