#pragma once
#include "WPage.h"
#include "XString.h"

class WRec /* record on WPage */
{
public:
	WRec() = default;
	WRec(unsigned char* data);
	WRec(WPage* wp);
	unsigned char N[3]{ 0, 0, 0 };
	unsigned char IR[3]{ 0, 0, 0 };
	XString X;
	int GetN(); // ASM
	void PutN(int NN); // ASM
	void PutIR(int II); // ASM
	unsigned short Comp(WRec* R); // ASM
	unsigned short Compare(const WRec& w) const;
	void Deserialize(unsigned char* data);
	size_t Serialize(unsigned char* buffer);
	bool operator == (const WRec& w) const;
	bool operator < (const WRec& w) const;
	bool operator > (const WRec& w) const;
};
