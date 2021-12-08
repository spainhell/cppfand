#pragma once
#include "../cppfand/constants.h"
#include "../cppfand/base.h"
#include "../cppfand/FieldDescr.h"

class CodingRdb
{
public:
	void CodeRdb(bool Rotate);
	void CompressTxt(WORD IRec, LongStr* s, char Typ);
	void Wr(BYTE c);
	void CodeF(bool rotate, WORD IRec, FieldDescr* F, char Typ);
	void CompressCRdb();

private:
	WORD l = 0;
	WORD posUDLI = 0;
	LongStr* ss = nullptr;
};

