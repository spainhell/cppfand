#pragma once
#include "WPage.h"
#include "XString.h"

class WRec /* record on WPage */
{
public:
	WRec() = default;
	WRec(unsigned char* data);
	WRec(WPage* wp);
	BYTE N[3]{ 0, 0, 0 };
	BYTE IR[3]{ 0, 0, 0 };
	XString X;
	longint GetN(); // ASM
	void PutN(longint NN); // ASM
	void PutIR(longint II); // ASM
	WORD Comp(WRec* R); // ASM
	WORD Compare(const WRec& w) const;
	void Deserialize(unsigned char* data);
	size_t Serialize(unsigned char* buffer);
	bool operator == (const WRec& w) const;
	bool operator < (const WRec& w) const;
	bool operator > (const WRec& w) const;
};
