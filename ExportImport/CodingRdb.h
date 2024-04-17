#pragma once

#include "../Core/FieldDescr.h"
#include "../DataEditor/EditReader.h"


class CodingRdb
{
public:
	void CodeRdb(EditD* edit, bool Rotate);
	void CompressTxt(WORD IRec, LongStr* s, char Typ);
	void Wr(BYTE c);
	void CodeF(bool rotate, WORD IRec, FieldDescr* F, char Typ);
	void CompressCRdb(EditD* edit);

private:
	WORD l = 0;
	WORD posUDLI = 0;
	LongStr* ss = nullptr;
};

