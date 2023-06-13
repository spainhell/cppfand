#pragma once
#include "../CppFand/constants.h"
#include "../CppFand/base.h"
#include "../CppFand/FieldDescr.h"

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

